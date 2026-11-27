FROM alpine:latest

RUN mkdir -p /shared

COPY lab2.sh /usr/local/bin/lab2.sh

RUN chmod +x /usr/local/bin/lab2.sh

CMD ["/usr/local/bin/lab2.sh"]

# Инструкция:
# 1. Один раз создать том, все контейнеры используют его
# docker volume create shared_vol

# 2. Запустить n контейнеров (10, 50)
# for i in $(seq 1 10); do
#   docker run -d --name "test_$i" \
    # Смонтировать том
#   --mount type=volume,src=shared_vol,destination=/shared \
#   lab2-container
# done
