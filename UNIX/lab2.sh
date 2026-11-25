#!/bin/sh
shared_dir="/shared"
lock_file="$shared_dir/.lock"

id_con=$(cat /proc/sys/kernel/random/uuid)
echo "Запуск контейнера $id_con"

count=0
exec 9>>"$lock_file"
while true; do
    count=$((count + 1))
    file_name=""

    flock -x 9

    i=1
    while true; do
        name=$(printf "%03d" $i)
        name_path="$shared_dir/$name"

        if [ ! -e "$name_path" ]; then
            file_name="$name"
            touch "$name_path"
            break
        fi

        i=$((i + 1))
    done

    flock -u 9

    echo "$id_con-$count" > "$shared_dir/$file_name"

    sleep 1
    rm -f "$shared_dir/$file_name"
    echo "Удаление $file_name"
    sleep 1
done