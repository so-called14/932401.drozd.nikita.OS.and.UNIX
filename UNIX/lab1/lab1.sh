#!/bin/sh
wrong_use=1
no_file=2
no_output=3
wrong_name=4
cant_tmp=5
cant_compile=6
wrong_type=7
cant_copy=8

if [ $# -ne 1 ]; then
    echo "Ошибка: укажите один файл" >&2
    exit $wrong_use
fi

file="$1"

if [ ! -f "$file" ]; then
    echo "Ошибка: файл '$file' не найден" >&2
    exit $no_file
fi

folder=$(cd "$(dirname "$file")" && pwd)
name=$(basename "$file")
path="$folder/$name"

output_name=$(grep -m 1 "Output:" "$path" | sed 's/.*Output:[[:space:]]*//')

if [ -z "$output_name" ]; then
    echo "Ошибка: в '$file' нет комментария 'Output:'" >&2
    exit $no_output
fi

case "$output_name" in
    */*|.|..)
        echo "Ошибка: недопустимое имя конечного файла: '$output_name'" >&2
        exit $wrong_name
        ;;
esac

if ! tmp=$(mktemp -d); then
    echo "Ошибка: не удалось создать временный каталог" >&2
    exit $cant_tmp
fi

exit_handler() {
    rc=$?
    trap - EXIT
    rm -rf "$tmp"
    exit $rc
}

trap exit_handler EXIT HUP INT QUIT PIPE TERM

type_file="${path##*.}"

if [ "$type_file" = "c" ]; then
    if ! cc "$path" -o "$tmp/$output_name"; then
        echo "Ошибка: не удалось скомпилировать C файл" >&2
        exit $cant_compile
    fi

elif [ "$type_file" = "cpp" ]; then
    
    if ! c++ "$path" -o "$tmp/$output_name"; then
        echo "Ошибка: не удалось скомпилировать C++ файл" >&2
        exit $cant_compile
    fi

elif [ "$type_file" = "tex" ]; then
    if ! pdflatex -interaction=nonstopmode -output-directory="$tmp" "$path" > /dev/null; then
        echo "Ошибка: не удалось скомпилировать tex файл" >&2
        exit $cant_compile
    fi

    tex_name=$(basename "$path" .tex)
    mv "$tmp/$tex_name.pdf" "$tmp/$output_name.pdf"
    output_name="$output_name.pdf"

else
    echo "Ошибка: неизвестный формат файла '$type_file'" >&2
    exit $wrong_type
fi


if ! cp "$tmp/$output_name" "$folder/$output_name"; then
    echo "Ошибка: не удалось скопировать '$output_name' в папку с исходником" >&2
    exit $cant_copy
fi

echo "Программа завершила работу: $folder/$output_name"
exit 0
