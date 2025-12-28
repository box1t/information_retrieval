import os
import re
import pymongo
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
# Используем неинтерактивный бэкэнд для сохранения в файл без вывода окна
matplotlib.use('Agg') 

from collections import Counter

# --- КОНСТАНТЫ И НАСТРОЙКИ ---
OUTPUT_DIRECTORY = '/home/snowwy/Documents/poor_monk/poor_monk/7th_term/infosearch/information_retrieval/src/lab3-5/plots'
FILENAME = 'zipf_law_analysis.png'
 
CONFIG = {
    'db': {
        'host': 'localhost',
        'port': 27017,
        'collection': 'documents'
    },
    'mandelbrot': {
        'B': 2.7,
        'G': 1.05
    }
}

# --- СЛОЙ ДАННЫХ ---
def get_mongo_data():
    user = os.getenv('MONGO_USER')
    password = os.getenv('MONGO_PASS')
    db_name = os.getenv('MONGO_DB', 'admin')
    
    uri = f"mongodb://{user}:{password}@{CONFIG['db']['host']}:{CONFIG['db']['port']}/"
    client = pymongo.MongoClient(uri)
    db = client[db_name]
    return db[CONFIG['db']['collection']].find({}, {"html": 1})

# --- СЛОЙ ЛОГИКИ ---
def tokenize(html_content):
    clean_text = re.sub(r'<[^>]+>', ' ', html_content)
    return re.findall(r'\w+', clean_text.lower())

def calculate_frequencies(cursor):
    all_tokens = []
    for doc in cursor:
        html = doc.get("html", "")
        all_tokens.extend(tokenize(html))
    
    counts = Counter(all_tokens)
    return sorted(counts.values(), reverse=True), len(all_tokens)

def get_theoretical_values(ranks, max_freq):
    zipf = [max_freq / r for r in ranks]
    b = CONFIG['mandelbrot']['B']
    g = CONFIG['mandelbrot']['G']
    p = max_freq * (1 + b)**g
    mandelbrot = [p / (r + b)**g for r in ranks]
    return zipf, mandelbrot

# --- СЛОЙ ПРЕДСТАВЛЕНИЯ (СОХРАНЕНИЕ) ---
def save_plot(ranks, freqs, zipf, mandelbrot):
    plt.figure(figsize=(10, 6))
    
    plt.loglog(ranks, freqs, 'b-', label='Реальные данные', linewidth=2)
    plt.loglog(ranks, zipf, 'r--', label='Закон Ципфа')
    
    label_m = f"Закон Мандельброта (B={CONFIG['mandelbrot']['B']}, G={CONFIG['mandelbrot']['G']})"
    plt.loglog(ranks, mandelbrot, 'g:', label=label_m)

    plt.title('Распределение терминов (log-log scale)')
    plt.xlabel('Ранг термина (r)')
    plt.ylabel('Частота встречи (f)')
    plt.legend()
    plt.grid(True, which="both", ls="-", alpha=0.2)

    # Логика сохранения в директорию
    os.makedirs(OUTPUT_DIRECTORY, exist_ok=True)
    graph_filepath = os.path.join(OUTPUT_DIRECTORY, FILENAME)
    
    plt.savefig(graph_filepath, dpi=300, bbox_inches='tight')
    plt.close() # Освобождаем память
    print(f"\nГрафик успешно сохранен: {graph_filepath}")

# --- ТОЧКА ВХОДА ---
if __name__ == "__main__":
    try:
        print("1. Подключение к MongoDB...")
        cursor = get_mongo_data()
        
        print("2. Анализ и расчеты...")
        frequencies, total_count = calculate_frequencies(cursor)
        
        if not frequencies:
            print("Ошибка: Данные не найдены.")
        else:
            ranks = np.arange(1, len(frequencies) + 1)
            zipf_vals, mandel_vals = get_theoretical_values(ranks, frequencies[0])
            
            print("3. Генерация и сохранение изображения...")
            save_plot(ranks, frequencies, zipf_vals, mandel_vals)
            
            print(f"Итого токенов: {total_count}")
            print(f"Размер словаря: {len(frequencies)}")
            
    except Exception as e:
        print(f"Произошла ошибка: {e}")