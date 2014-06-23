zero-unset-multi-backuper
=========================

Мультипроцессный бэкапер на основе алгоритма сжатия с помощью исключения нулевых байтов из исходника и составления битовой карты.

Испытание
=========================

Можно протестировать программу, используя командный файл run.cmd.
Он произведет компиляцию и запустит программу: бэкап файла image.iso и, затем, его восстановление с новым именем res.iso.
В образе находятся 3 картинки и 1 исходник программы Hellow World.

Инструкция
=========================

Бэкап.

Для создания бэкапа, программу нужно запускать с 4-мя параметрами,
первый из которых должен быть -c [путевое имя образа для бэкапа], а два
других задаваться аналогичным образом. Указание путевого имени 
хранения файла битовой карты: -b [путевое имя файла битовой карты].
Указание путевого имени хранения файла бэкапа:
-B [путевое имя файла бэкапа], и -p [количество процессов].
Пример: mkbup -c /dev/hdb1 -b /home/user/bitmap -B /home/user/backup -p 12

Восстановление.

Для восстановления образа, нужно запускать программу с 4-мя параметрами,
первый из которых должен быть -r [путевое имя будущего восстановленого образа].
Остальные параметры задаются аналогичным образом, что и в пункте 1.
Пример: mkbup -r /dev/hdb1 -b /home/user/bitmap -B /home/user/backup -p 12
