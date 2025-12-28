
# Поисковый робот

## Принцип работы **`lab2`**

Параметры конфигурации для __logic__ из `config.yaml`:

```yaml
delay: 1.0
workers_count: 2
max_pages: 50
max_depth: 1
```

## Запуск модуля

__Виртуального окружения__:
```sh
python -m venv vvenvv
source ./vvenvv/bin/activate
pip install requirements.txt
```

__Базы данных__:
```shell
sudo docker compose down -v 

sudo docker compose up -d
```


## Запуск робота
Для начала нужно инициализировать подмодули:

`crawler.py` (поисковый робот):
```sh
python3 src/lab2/crawler.py src/lab2/config.yaml
```

`сheck_db.py` (проверка состава документов):
```sh
python3 src/lab2/check_db.py
```