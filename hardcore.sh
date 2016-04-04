#!/bin/bash

BOLD=$(tput bold)
NORMAL=$(tput sgr0)

egrep -v '^#' ~/dayz_servers_hardcore | while IFS=, read NAME IP PORT; do
    INFO=$(arma2serverinfo ${IP} ${PORT} 2>/dev/null)
    COUNT=$(echo "$INFO" | awk '/^PLAYERS/ {print $NF}')
    TIME=$(echo "$INFO" | awk -F, '/^KEYWORDS/ {print $NF}')
    printf "%25s (%2d) %s\n" "${BOLD}${NAME}${NORMAL}" ${COUNT} ${TIME}
done

