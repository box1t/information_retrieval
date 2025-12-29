import os
import sys
import subprocess
import json
from pathlib import Path
from flask import Flask, request, render_template, jsonify

app = Flask(__name__)



def search_documents(query: str) -> dict:
    """
    Выполнение поиска через CLI
    
    Args:
        query: поисковый запрос
        
    Returns:
        словарь с результатами поиска
    """
    if not query or not query.strip():
        return {
            'error': 'Пустой запрос',
            'count': 0,
            'doc_ids': []
        }
    
    cli_path = Path(CLI_BIN)
    if not cli_path.exists():
        alt_path = Path(__file__).parent.parent / 'core' / 'build' / 'cli' / 'search_cli'
        if alt_path.exists():
            cli_path = alt_path
        else:
            return {
                'error': 'CLI бинарник не найден. Запустите: ./scripts/build_all.sh',
                'count': 0,
                'doc_ids': []
            }
    
    index_path = Path(INDEX_PATH)
    if not index_path.exists():
        alt_index = Path(__file__).parent.parent / 'core' / 'index' / 'boolean_index.bin'
        if alt_index.exists():
            index_path = alt_index
        else:
            return {
                'error': 'Индекс не найден. Постройте индекс: ./scripts/build_index.sh',
                'count': 0,
                'doc_ids': []
            }
    
    try:
        result = subprocess.run(
            [str(cli_path), str(index_path), query],
            capture_output=True,
            text=True,
            encoding='utf-8',
            timeout=30
        )
        
        if result.returncode != 0:
            return {
                'error': 'Ошибка выполнения поиска',
                'message': result.stderr,
                'count': 0,
                'doc_ids': []
            }
        
        output = result.stdout
        doc_ids = []
        
        lines = output.split('\n')
        in_results = False
        
        for line in lines:
            line = line.strip()
            
            if 'Найдено документов:' in line:
                try:
                    count_str = line.split(':')[1].strip()
                    count = int(count_str)
                except:
                    count = 0
            
            if 'Первые' in line and 'результатов:' in line:
                in_results = True
                continue
            
            if in_results and line:
                parts = line.replace('...', '').split(',')
                for part in parts:
                    part = part.strip()
                    if part and part.isdigit():
                        doc_ids.append(int(part))
                if '...' in line:
                    break
        
        if not doc_ids:
            for line in lines:
                parts = line.split(',')
                for part in parts:
                    part = part.strip()
                    if part.isdigit() and len(part) <= 6:
                        doc_ids.append(int(part))
        
        return {
            'query': query,
            'count': len(doc_ids),
            'doc_ids': doc_ids
        }
        
    except subprocess.TimeoutExpired:
        return {
            'error': 'Таймаут выполнения поиска',
            'count': 0,
            'doc_ids': []
        }
    except Exception as e:
        return {
            'error': 'Ошибка при выполнении поиска',
            'message': str(e),
            'count': 0,
            'doc_ids': []
        }

INDEX_PATH = os.environ.get('INDEX_PATH')
DOC_DIR = os.environ.get('DOC_DIR')
CLI_BIN = os.environ.get('CLI_BIN')
RESULTS_PER_PAGE = 20

def get_document_info(doc_id: int) -> dict:
    """
    Получение информации о документе
    
    Args:
        doc_id: идентификатор документа
        
    Returns:
        словарь с информацией о документе
    """
    corpus_path = Path(DOC_DIR)
    if not corpus_path.exists():
        corpus_path = Path(__file__).parent.parent / 'corpus'
    
    pattern = f"doc_{doc_id:05d}.txt"
    files = list(corpus_path.glob(pattern))
    
    if not files:
        return {
            'id': doc_id,
            'error': 'Документ не найден'
        }
    
    filepath = files[0]
    
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
            
        lines = content.split('\n')
        title = f"Документ {doc_id}"
        url = ""
        word_count = 0
        
        for line in lines:
            if line.startswith('TITLE:'):
                title = line.replace('TITLE:', '').strip()
            elif line.startswith('URL:'):
                url = line.replace('URL:', '').strip()
            elif line.startswith('WORDS:'):
                try:
                    word_count = int(line.replace('WORDS:', '').strip())
                except:
                    pass
            elif line.startswith('=' * 80):
                break
        
        text_start = content.find('=' * 80)
        if text_start != -1:
            text = content[text_start + 80:].strip()
        else:
            text = content
        
        preview = text[:300].strip()
        if len(text) > 300:
            preview += "..."
        
        return {
            'id': doc_id,
            'title': title,
            'url': url,
            'word_count': word_count,
            'preview': preview,
            'filename': filepath.name
        }
    except Exception as e:
        return {
            'id': doc_id,
            'error': str(e)
        }


def highlight_terms(text: str, query: str) -> str:
    """
    Подсветка найденных терминов в тексте
    
    Args:
        text: текст для подсветки
        query: поисковый запрос
        
    Returns:
        текст с HTML подсветкой
    """
    if not query or not text:
        return text
    
    import re
    words = re.findall(r'\b\w+\b', query.lower())
    
    highlighted = text
    for word in words:
        if len(word) > 2:  # Игнорировать слишком короткие слова
            pattern = re.compile(re.escape(word), re.IGNORECASE)
            highlighted = pattern.sub(
                lambda m: f'<mark>{m.group()}</mark>',
                highlighted
            )
    
    return highlighted


@app.route('/')
def index():
    """Главная страница с формой поиска"""
    return render_template('index.html')


@app.route('/search', methods=['POST', 'GET'])
def search():
    """Обработка поискового запроса"""
    if request.method == 'GET':
        query = request.args.get('q', '')
        page = int(request.args.get('page', 1))
    else:
        query = request.form.get('query', '')
        page = int(request.form.get('page', 1))
    
    if not query:
        return render_template('index.html', error='Введите поисковый запрос')
    
    results = search_documents(query)
    
    if 'error' in results and results.get('count', 0) == 0:
        return render_template('index.html', error=results.get('error', 'Ошибка поиска'), query=query)
    
    all_doc_ids = results.get('doc_ids', [])
    total_results = len(all_doc_ids)
    
    start_idx = (page - 1) * RESULTS_PER_PAGE
    end_idx = start_idx + RESULTS_PER_PAGE
    page_doc_ids = all_doc_ids[start_idx:end_idx]
    
    documents = []
    for doc_id in page_doc_ids:
        doc_info = get_document_info(doc_id)
        if 'error' not in doc_info:
            # Подсветить термины в превью
            doc_info['preview'] = highlight_terms(doc_info.get('preview', ''), query)
        documents.append(doc_info)
    
    total_pages = (total_results + RESULTS_PER_PAGE - 1) // RESULTS_PER_PAGE if total_results > 0 else 1
    
    return render_template('index.html', 
                         query=query,
                         count=total_results,
                         documents=documents,
                         page=page,
                         total_pages=total_pages,
                         has_prev=page > 1,
                         has_next=page < total_pages)


@app.route('/api/search', methods=['GET', 'POST'])
def api_search():
    """API endpoint для поиска (JSON)"""
    if request.method == 'POST':
        data = request.get_json()
        query = data.get('query', '') if data else ''
    else:
        query = request.args.get('q', '')
    
    if not query:
        return jsonify({'error': 'Query parameter required'}), 400
    
    results = search_documents(query)
    return jsonify(results)


@app.route('/api/document/<int:doc_id>')
def api_document(doc_id):
    """API endpoint для получения информации о докуменte"""
    doc_info = get_document_info(doc_id)
    return jsonify(doc_info)


if __name__ == '__main__':
    port = int(os.environ.get('PORT', 5000))
    debug = os.environ.get('DEBUG', 'False').lower() == 'true'
    
    print(f"Запуск веб-сервера на http://localhost:{port}")
    print(f"INDEX_PATH: {INDEX_PATH}")
    print(f"DOC_DIR: {DOC_DIR}")
    print(f"CLI_BIN: {CLI_BIN}")
    
    app.run(host='0.0.0.0', port=port, debug=debug)