#!/bin/bash

for var in "$@"
do
    case $var in
        -l|--list)
            echo "c++"
            exit 0
            ;;
        -h|--help)
            echo "Usage: callbacks.sh [--list]"
            exit 0
            ;;
        -v|--version)
            printf "%s, version %s\n" "FPPPushoverNotifications" "0.4.1"
            exit 0
            ;;
        --)
            break
            ;;
        *)
            printf "Unknown option %s\n" "$var"
            exit 1
            ;;
    esac
done

exit 0
