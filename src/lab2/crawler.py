import asyncio
import aiohttp
import yaml
import time
import os
from motor.motor_asyncio import AsyncIOMotorClient
from datetime import datetime
from dotenv import load_dotenv
import sys

load_dotenv()

class WikiCrawler:
    def __init__(self, config_path):
        with open(config_path, 'r') as f:
            self.config = yaml.safe_load(f)
        
        # Подключение к БД через env
        user = os.getenv('MONGO_USER')
        password = os.getenv('MONGO_PASS')
        db_name = os.getenv('MONGO_DB')
        uri = f"mongodb://{user}:{password}@{self.config['db']['host']}:{self.config['db']['port']}/"
        
        self.client = AsyncIOMotorClient(uri)
        self.db = self.client[db_name]
        self.collection = self.db['documents']
        self.queue_col = self.db['queue'] # Основная очередь и processing

        self.max_pages = self.config['logic'].get('max_pages', 50)
        self.current_count = 0

    async def init_queue(self, reset_all=False):
        """Восстановление очереди при перезапуске (Processing -> Main)"""
        # Если есть документы в статусе 'processing', возвращаем их в 'new'
        res = await self.queue_col.update_many(
            {"status": "processing"},
            {"$set": {"status": "new"}}
        )
        if reset_all:
            print("DEBUG: Сброс всех задач для переобкачки...")
            await self.queue_col.update_many({"status": "done"}, {"$set": {"status": "new"}})
        
        if res.modified_count > 0:
            print(f"Восстановлено из processing: {res.modified_count} ссылок")

        # Добавляем стартовые страницы из конфига
        for target in self.config['targets']:
            url = f"{target['api_url']}?page={target['start_page']}"
            if await self.queue_col.count_documents({"url": url}) == 0:
                await self.queue_col.insert_one({
                    "url": url,
                    "title": target['start_page'],
                    "api_url": target['api_url'],
                    "source": target['name'],
                    "status": "new"
                })

    async def fetch_page_api(self, session, task):
        """Запрос к action=parse для получения контента"""
        params = {
            "action": "parse",
            "page": task['title'],
            "disableeditsection": 1,
            "formatversion": 2,
            "format": "json"
        }
        async with session.get(task['api_url'], params=params) as resp:
            if resp.status == 200:
                data = await resp.json()
                return data.get('parse', {})
            return None

    async def check_updates(self, session, tasks):
        """Запрос к action=query для проверки timestamp (сразу для группы страниц)"""
        titles = "|".join([t['title'] for t in tasks])
        params = {
            "action": "query",
            "prop": "revisions",
            "rvslots": "*",
            "titles": titles,
            "rvprop": "timestamp",
            "format": "json",
            "formatversion": 2
        }
        # Здесь логика сравнения timestamp из БД и API
        # Если в БД timestamp < API_timestamp -> качаем заново

    async def worker(self, worker_id):
        async with aiohttp.ClientSession() as session:
            while True:
                # ГЛОБАЛЬНЫЙ ЛИМИТ: проверяем сколько уже в базе
                self.current_count = await self.collection.count_documents({})
                if self.current_count >= self.max_pages:
                    print(f"Worker-{worker_id}: Лимит в {self.max_pages} страниц достигнут. Завершение.")
                    break

                task = await self.queue_col.find_one_and_update(
                    {"status": "new"},
                    {"$set": {"status": "processing"}},
                    sort=[("depth", 1), ("_id", 1)] # BFS: сначала качаем те, что ближе к главной
                )

                if not task:
                    print(f"DEBUG: Воркер {worker_id} спит (очередь пуста)")
                    await asyncio.sleep(2)
                    continue
                print(f"Worker-{worker_id} взял в работу: {task['title']}")

                # ЛИМИТ ГЛУБИНЫ: например, не дальше 2 кликов от главной
                if task.get('depth', 0) > self.config['logic'].get('max_depth', 2):
                    await self.queue_col.delete_one({"_id": task["_id"]})
                    continue

                try:
                    page_data = await self.fetch_page_api(session, task)
                    if page_data:
                        # Сохраняем статью
                        await self.collection.update_one(
                            {"url": task['url']},
                            {"$set": {
                                "url": task['url'],
                                "html": page_data.get('text'),
                                "name": task['source'],
                                "timestamp": time.time(),
                                "depth": task.get('depth', 0)
                            }},
                            upsert=True
                        )

                        # Извлекаем ссылки с лимитом
                        links = page_data.get('links', [])
                        added_on_this_page = 0
                        for link in links:
                            #ns: 0 - это только статьи, без 'Обсуждение:', 'Файл:' и т.д.
                            if link.get('ns') == 0 and added_on_this_page < 20: 
                                new_title = link['title']
                                new_url = f"{task['api_url']}?page={new_title}"
                                
                                if await self.queue_col.count_documents({"url": new_url}) == 0:
                                    await self.queue_col.insert_one({
                                        "url": new_url,
                                        "title": new_title,
                                        "api_url": task['api_url'],
                                        "source": task['source'],
                                        "status": "new",
                                        "depth": task.get('depth', 0) + 1 # Увеличиваем глубину
                                    })
                                    added_on_this_page += 1

                        await self.queue_col.update_one(
                            {"_id": task["_id"]},
                            {"$set": {
                                "status": "done",
                                "last_crawled": time.time()
                    }}
                )
                except Exception as e:
                    print(f"Error: {e}")
                    await self.queue_col.update_one({"_id": task["_id"]}, {"$set": {"status": "problematic"}})
                
                await asyncio.sleep(self.config['logic']['delay'])

    async def run(self):
        print("DEBUG: Подключение к БД...")
        # False - нет переобкачки по умолчанию
        await self.init_queue(reset_all=False)
        
        # Проверим, есть ли вообще задачи
        count = await self.queue_col.count_documents({"status": "new"})
        print(f"DEBUG: В очереди найдено задач: {count}")

        print(f"DEBUG: Запуск {self.config['logic']['workers_count']} воркеров...")
        workers = [
            asyncio.create_task(self.worker(i)) 
            for i in range(self.config['logic']['workers_count'])
        ]
        await asyncio.gather(*workers)

if __name__ == "__main__":
    if len(sys.argv) < 2:
            print("Использование: python3 src/lab2/crawler.py src/lab2/config.yaml")
    else:
        # sys.argv[1] — это путь, который вы передаете в консоли
        crawler = WikiCrawler(sys.argv[1]) 
        asyncio.run(crawler.run())