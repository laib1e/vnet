# vnet — виртуальный сетевой интерфейс (модуль ядра Linux)

Модуль ядра Linux, создающий виртуальный сетевой интерфейс `vnet0`, который отвечает на ICMP Echo Request (ping). IPv4-адрес задаётся через procfs (`/proc/vnet`).

## Требования

- Linux (протестировано на Ubuntu 24.04, ядро 6.17)
- Заголовки ядра: linux-headers-6.17.0-19-generic

Установка зависимостей:

```bash
sudo apt install build-essential linux-headers-$(uname -r)
```

## Сборка

```bash
make
```

Сборка выполняется от обычного пользователя. Установка — от root.

## Установка

```bash
sudo make install           # копирует vnet.ko в /lib/modules/.../extra/
sudo make install-service   # устанавливает systemd unit
```

## Удаление

Cначала сервис (выгружает модуль), потом файлы:

```bash
sudo make uninstall-service && sudo make uninstall
```

## Использование

### Через systemd

```bash
sudo systemctl start vnet    # загрузка модуля, настройка интерфейса и маршрута
sudo systemctl stop vnet     # выгрузка модуля
```

IP-адрес и маршрут задаются в `vnet.service`.

### Вручную

```bash
sudo insmod vnet.ko
sudo ip link set vnet0 up
echo "10.0.0.1" | sudo tee /proc/vnet
sudo ip route add 10.0.0.1/32 dev vnet0
ping 10.0.0.1
```

Чтение текущего IP:

```bash
cat /proc/vnet
```

## Архитектура

### Модуль ядра (`vnet.c`)

Модуль состоит из трёх частей:

**Виртуальный сетевой интерфейс.** Создаётся через `alloc_netdev` / `register_netdev`. Флаг `IFF_NOARP` отключает ARP — для виртуального интерфейса он не нужен и создаёт лишние проблемы (ядро отправляет ARP-запросы вместо ICMP).

**procfs (`/proc/vnet`).** Файл для чтения и записи IPv4-адреса. При записи строка парсится через `in4_pton` и сохраняется в двух форматах: строковом (`char[16]`) для отображения и бинарном (`__be32`) для сравнения с адресом в пакетах. Реализована валидация ввода и защита от переполнения буфера.

**Обработка ICMP в `start_xmit`.** Когда ядро передаёт пакет через интерфейс, `start_xmit` проверяет: EtherType == IP → Protocol == ICMP → Type == Echo Request → Destination IP совпадает с заданным. Если все условия выполнены — формируется Echo Reply: меняются местами MAC и IP-адреса, тип ICMP меняется на 0 (reply), пересчитываются контрольные суммы. Пакет возвращается в сетевой стек через `netif_rx`. Остальные пакеты отбрасываются.

### systemd unit (`vnet.service`)

Unit типа `oneshot` с `RemainAfterExit=yes`. При старте: загружает модуль, поднимает интерфейс, задаёт IP через procfs, добавляет маршрут. При остановке: выгружает модуль (`modprobe -r`), интерфейс и маршрут удаляются автоматически.

### Makefile

Цели: `all` (сборка), `clean` (очистка), `install`/`uninstall` (модуль), `install-service`/`uninstall-service` (systemd unit).

## Известные особенности

- NetworkManager может перехватывать управление интерфейсом `vnet0`. Решение: создать `/etc/NetworkManager/conf.d/vnet.conf` с содержимым `[keyfile]\nunmanaged-devices=interface-name:vnet*` и перезапустить NetworkManager.
- Описание найденных и исправленных багов — в отдельных ветках репозитория.

## Лицензия

GPL v2