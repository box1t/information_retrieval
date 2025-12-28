import os
import time
from pymongo import MongoClient
from dotenv import load_dotenv
from datetime import datetime

load_dotenv()

def check_results():
    user = os.getenv('MONGO_USER')
    password = os.getenv('MONGO_PASS')
    uri = f"mongodb://{user}:{password}@localhost:27017/"
    
    client = MongoClient(uri)
    db = client[os.getenv('MONGO_DB')]
    collection = db['documents']

    print(f"--- Проверка базы данных ---")
    total = collection.count_documents({})
    print(f"Всего документов в базе: {total}")

    if total == 0:
        print("База пуста!")
        return

    cursor = collection.find().sort("timestamp", -1).limit(3)

    for i, doc in enumerate(cursor):
        print(f"\n[Документ #{i+1}]")
        print(f"URL:    {doc.get('url')}")
        print(f"Source: {doc.get('name')}")
        
        ts = doc.get('timestamp')
        readable_date = datetime.fromtimestamp(ts).strftime('%Y-%m-%d %H:%M:%S')
        print(f"Time:   {ts} (дата: {readable_date})")
        
        html = doc.get('html', '')
        html_snippet = html[:100].replace('\n', ' ')
        print(f"HTML:   {html_snippet}...")
        print(f"Размер HTML: {len(html)} символов")

if __name__ == "__main__":
    check_results()