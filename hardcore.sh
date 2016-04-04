#!/bin/bash
#
# Simple wrapper script for checking population and time of day of DayZ servers
# Input file is CSV format NAME,IP,QUERY PORT
#
# Example output:
# $ hardcore.sh
# DayZUnderground (50) 10:24
#  Oldschool DayZ (30) 14:32
#           UN #1 ( 8) 14:42
#           UN #2 ( 7) 14:25

BOLD=$(tput bold)
NORMAL=$(tput sgr0)

egrep -v '^#' ~/dayz_servers_hardcore | while IFS=, read NAME IP PORT; do
    INFO=$(arma2serverinfo ${IP} ${PORT} 2>/dev/null)
    COUNT=$(echo "$INFO" | awk '/^PLAYERS/ {print $NF}')
    TIME=$(echo "$INFO" | awk -F, '/^KEYWORDS/ {print $NF}')
    printf "%25s (%2d) %s\n" "${BOLD}${NAME}${NORMAL}" ${COUNT} ${TIME}
done

