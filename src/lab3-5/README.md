
# Токенизация, Закон Ципфа, Стемминг

## Токенизация

- Установка драйвера Mongodb [согласно инструкции](https://www.mongodb.com/docs/languages/cpp/cpp-driver/current/get-started/#std-label-cpp-get-started)

## Стемминг

- установка stemming: `sudo apt-get install libstemmer-dev`


### Сборка проекта (начало в корневой папке)


```sh
cd src/lab3-5/


mkdir build
cmake ..
cmake --build .
export $(grep -v '^#' .env | xargs) && ./tokenizer_lab


rm -rf build
```
cd build

## Булев индекс

- проверка создания бинарных файлов: `ls -lh *.bin` 