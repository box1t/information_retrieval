import xml.etree.ElementTree as ET
import os

def analyze_wiki_dump(file_path):
    count = 0
    total_raw_size = os.path.getsize(file_path)
    total_text_length = 0
    
    print(f"Начинаю анализ файла: {file_path}...")
    context = ET.iterparse(file_path, events=('end',))
    
    for event, elem in context:
        tag = elem.tag.split('}')[-1]
        
        if tag == 'page':
            count += 1
            for child in elem.iter():
                child_tag = child.tag.split('}')[-1]
                if child_tag == 'text':
                    if child.text:
                        total_text_length += len(child.text)
                    break 
            elem.clear()
            
            if count % 5000 == 0:
                print(f"Обработано {count} документов...")

    avg_text_size = total_text_length / count if count > 0 else 0
    raw_mb = total_raw_size / (1024*1024)
    text_mb = total_text_length / (1024*1024)
    
    print("\n--- ИТОГОВАЯ СТАТИСТИКА ---")
    print(f"Размер 'сырого' файла: {raw_mb:.2f} МБ")
    print(f"Количество документов: {count}")
    print(f"Общий объем текста: {text_mb:.2f} МБ")
    print(f"Средний объем текста в документе: {avg_text_size:.2f} символов")

analyze_wiki_dump('src/lab1/gtawiki_pages_current.xml')