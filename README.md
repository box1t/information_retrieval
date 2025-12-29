# Информационный поиск

## Реализованные возможности функционирования

| Модули репозитория | Краткое описание |
| :------------------------------------ | :------------------------------------ |
| lab1 | Добыча корпуса документов |
| lab2 | Поисковый робот |
| lab3-5 | Токенизация, Закон Ципфа, Стемминг |
| lab7 | Булев индекс |
| lab8 | Булев поиск |


```
information_retrieval/
├── src/
│   ├── lab1/
│   │   └── analyze_wiki_dump.py
│   ├── lab2/
│   │   ├── config.yaml
│   │   ├── check_db.py 
│   │   └── crawler.py 
│   └── lab3-8/
│       ├── plots/
│       │   ├── zypf_law.py
│       │   └── zipf_law_analysis.png
│       ├── include/
│       │   ├── indexer.h
│       │   ├── search.h
│       │   ├── types.h
│       │   └── tokenizer.h
│       └── src/
│           ├── indexer.cpp
│           ├── cli.cpp
│           ├── search.cpp
│           └── main.cpp
│
├── reports/
│   ├── report.pdf
│   └── report.tex
├── docker-compose.yml
└── requirements.txt
```
