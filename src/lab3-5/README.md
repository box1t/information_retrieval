
# Токенизация, Закон Ципфа, Стемминг

## Токенизация

### Сборка проекта (начало в корневой папке)

- Установка драйвера Mongodb [согласно инструкции](https://www.mongodb.com/docs/languages/cpp/cpp-driver/current/get-started/#std-label-cpp-get-started)

```sh
cd src/lab3-5/


mkdir build
cmake ..
cmake --build .
export $(grep -v '^#' .env | xargs) && ./tokenizer_lab


rm -rf build
```
cd build
