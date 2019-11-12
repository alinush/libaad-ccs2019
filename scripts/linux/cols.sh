if [ $# -lt 2 ]; then
    echo "Usage: $0 <file.csv> <cols>"
    echo
    echo "You can use 'all' for <cols>."
    echo
    echo "Example: $0 some-file.csv 1,4,5"
    echo "Example: $0 some-file.csv all"
    
fi

if [ -f "$1" ]; then
    echo
    echo "Fields in '$1':"
    i=0; 
    for f in `cat "$1" | head -n 1 | tr ',' '\n'`; do 
        i=$((i+1))
        echo $i - $f
    done
    echo
fi

if [ $# -lt 2 ]; then
    exit 1 
fi

hscroll_size=20
file=$1

if [ "$2" == "all" ]; then
    cols=
    max=$(($i-1))
    cols="1"
    for c in `seq 2 $max`; do
        cols="$cols,$c"
    done 
    cols="$cols,$i"
else
    cols=$2
fi

echo "Columns: $cols"

cat "$file" | cut -d',' -f$cols | sed -e 's/,,/, ,/g' | column -s, -t | less -#$hscroll_size -N -S
