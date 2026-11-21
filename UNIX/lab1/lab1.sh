#!/bin/sh
wrong_use=1
no_file=2
no_output=3
wrong_name=4
cant_tmp=5
cant_switch=6
cant_compile=7
cant_rename=8
wrong_type=9
cant_copy=10

start_folder=$(pwd) || exit $no_file

tmp=""

exit_handler() {
    rc=$?
    trap - EXIT
    if [ -n "$tmp" ] && [ -d "$tmp" ]; then
        rm -rf "$tmp"
    fi
    exit $rc
}

trap exit_handler EXIT
trap 'exit 1' HUP
trap 'exit 2' INT
trap 'exit 3' QUIT
trap 'exit 4' PIPE
trap 'exit 5' TERM

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

output_name=$(grep -E -m1 '^[[:space:]]*(//|#|%)[[:space:]]*Output:' "$path" \
    | sed -E 's/.*Output:[[:space:]]*//' \
    | tr -d '\r' \
    | sed -E 's/[[:space:]]+$//')

if [ -z "$output_name" ]; then
    echo "Ошибка: в '$file' нет комментария 'Output:'" >&2
    exit $no_output
fi

case "$output_name" in
    */*|.|..|-*)
        echo "Ошибка: недопустимое имя конечного файла: '$output_name'" >&2
        exit $wrong_name
        ;;
esac

tmp=$(mktemp -d "${TMPDIR:-/tmp}/lab1.XXXXXX") || {
    echo "Ошибка: не удалось создать временный каталог" >&2
    exit $cant_tmp
}

cd "$tmp" || {
    echo "Ошибка: не удалось перейти во временный каталог" >&2
    exit $cant_switch
}

type_file="${path##*.}"

if [ "$type_file" = "c" ]; then
    if ! cc "$path" -o "$output_name"; then
        echo "Ошибка: не удалось скомпилировать C файл" >&2
        exit $cant_compile
    fi

elif [ "$type_file" = "cpp" ]; then
    if ! c++ "$path" -o "$output_name"; then
        echo "Ошибка: не удалось скомпилировать C++ файл" >&2
        exit $cant_compile
    fi

elif [ "$type_file" = "tex" ]; then
    if ! pdflatex -interaction=nonstopmode "$path" > /dev/null; then
        echo "Ошибка: не удалось скомпилировать tex файл" >&2
        exit $cant_compile
    fi

    tex_name=$(basename "$path" .tex)
    
    case "$output_name" in
        *.pdf) final_name="$output_name" ;;
        *)     final_name="$output_name.pdf" ;;
    esac

    if ! mv "$tex_name.pdf" "$final_name"; then
        echo "Ошибка: не удалось переименовать PDF" >&2
        exit $cant_rename
    fi
    output_name="$final_name"

else
    echo "Ошибка: неизвестный формат файла '$type_file'" >&2
    exit $wrong_type
fi

if ! cp "$output_name" "$start_folder/$output_name"; then
    echo "Ошибка: не удалось скопировать '$output_name' в '$start_folder'" >&2
    exit $cant_copy
fi

echo "Программа завершила работу: $start_folder/$output_name"
exit 0
