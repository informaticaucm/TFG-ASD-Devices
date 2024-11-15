@REM docker run --rm -v .:/project -w /project -e HOME=/tmp espressif/idf idf.py fullclean
docker run --rm -v .:/project -w /project -e HOME=/tmp espressif/idf:v5.3.1 idf.py build
@REM Seteada la versión a v5.3.1 de esp-idf