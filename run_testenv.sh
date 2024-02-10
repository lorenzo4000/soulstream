#!/bin/bash

## run local soulfind server and one nicotine client for development and debugging

konsole -e "soulfind ./testenv/soulfind/db.db" &
./testenv/nicotine/nicotine-plus/nicotine -c ./testenv/nicotine/conf
