gcc mkbup.c -o mkbup
gcc proc.c -o proc
export PATH=.:$PATH
#mkbup -c /dev/hdc6 -b bitmap -B backup -p 5
mkbup -c image.iso -b bitmap -B backup -p 5
#mkbup -r /dev/hdc6 -b bitmap -B backup -p 5
mkbup -r res.iso -b bitmap -B backup -p 5
