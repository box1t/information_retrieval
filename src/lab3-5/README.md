
# Лабораторные 3,4,5,7,8


## ЛР-3. Токенизация.

- Установка драйвера Mongodb [согласно инструкции](https://www.mongodb.com/docs/languages/cpp/cpp-driver/current/get-started/#std-label-cpp-get-started)

## ЛР-4. Закон Ципфа.

![alt text](<plots/zipf_law_analysis.png>)

## ЛР-5. Стемминг

- установка stemming: `sudo apt-get install libstemmer-dev`



## ЛР-7. Булев индекс.

- проверка создания бинарных файлов: `ls -lh *.bin` 

## ЛР-8. Булев поиск.



## Сборка проекта (начало в корневой папке)


```sh
cd src/lab3-5/


mkdir build
cmake ..
cmake --build .
export $(grep -v '^#' .env | xargs) && ./tokenizer_lab

./search_cli queries.txt

rm -rf build
```
cd build
