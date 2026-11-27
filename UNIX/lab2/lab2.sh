#!/bin/sh
shared_dir="/shared"
lock_file="$shared_dir/.lock"

id_con="$(date +%s)-$$"
echo "Запуск контейнера $id_con"

count=0
exec 9>>"$lock_file"
set -C
while true; do
    count=$((count + 1))
    file_name=""

    flock -x 9
    
    i=1
    while true; do
        name=$(printf "%03d" $i)
        name_path="$shared_dir/$name"

        if echo "$id_con-$count" > "$name_path" 2>/dev/null; then
            file_name="$name"
            break
        fi

        i=$((i + 1))
    done

    flock -u 9

    sleep 1
    rm -f "$shared_dir/$file_name"
    echo "Удаление $file_name"
    sleep 1
done
