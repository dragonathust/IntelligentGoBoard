#!/bin/bash

RUN_DIR="/tmp/igo"
CONFIG_FILE_DIR="/root/conf"
CONFIG_FILE_GO="go.conf"
CONFIG_FILE_GOMOKU="gomoku.conf"
CONFIG_FILE_MODE="igo.conf"

mode=0
go_player_color=0
gomoku_player_color=0
ref_init=1

if [ ! -d $CONFIG_FILE_DIR ]; then
    mkdir -p $CONFIG_FILE_DIR
fi

if [ -e $CONFIG_FILE_DIR"/"$CONFIG_FILE_MODE ]; then
    mode=`cat $CONFIG_FILE_DIR"/"$CONFIG_FILE_MODE`
fi

if [ -e $CONFIG_FILE_DIR"/"$CONFIG_FILE_GO ]; then
    go_player_color=`cat $CONFIG_FILE_DIR"/"$CONFIG_FILE_GO`
fi

if [ -e $CONFIG_FILE_DIR"/"$CONFIG_FILE_GOMOKU ]; then
    gomoku_player_color=`cat $CONFIG_FILE_DIR"/"$CONFIG_FILE_GOMOKU`
fi

while true
do

if [ $mode -eq 0 ]; then
echo "run Go..."
  aplay $RUN_DIR/sound/go.wav
  if [ $go_player_color -eq 0 ]; then
  aplay $RUN_DIR/sound/playblack.wav
  else
  aplay $RUN_DIR/sound/playwhite.wav
  fi
  $RUN_DIR/igo_gnugo -b $RUN_DIR/gnugo_arm64 -c $go_player_color -r $ref_init -p /dev/ttyACM0 > /tmp/igo.log 2>&1
else
echo "run Gomoku..."
  aplay $RUN_DIR/sound/gomoku.wav
  if [ $gomoku_player_color -eq 0 ]; then
  aplay $RUN_DIR/sound/playblack.wav
  else
  aplay $RUN_DIR/sound/playwhite.wav
  fi
  $RUN_DIR/igo_gomoku -b $RUN_DIR/pbrain-carbon -c $gomoku_player_color -r $ref_init -p /dev/ttyACM0 > /tmp/igo.log 2>&1
fi

case $? in
  0) echo "restart same game"
     let ref_init=0
     aplay $RUN_DIR/sound/replay.wav
     continue
         ;;
  1) echo "restart same game with different player color"
     let ref_init=0
     if [ $mode -eq 0 ]; then
         let go_player_color=!go_player_color
     echo $go_player_color > $CONFIG_FILE_DIR"/"$CONFIG_FILE_GO
     else
         let gomoku_player_color=!gomoku_player_color
     echo $gomoku_player_color > $CONFIG_FILE_DIR"/"$CONFIG_FILE_GOMOKU
     fi
     aplay $RUN_DIR/sound/replay.wav
         ;;
  2) echo "restart next game"
     let ref_init=0
     let mode=!mode
     echo $mode > $CONFIG_FILE_DIR"/"$CONFIG_FILE_MODE
     aplay $RUN_DIR/sound/replay.wav
         ;;
  3) echo "win - restart same game"
     let ref_init=0
     aplay $RUN_DIR/sound/youwin.wav
     aplay $RUN_DIR/sound/winmusic.wav
     sleep 3
     aplay $RUN_DIR/sound/replay.wav
     continue
         ;;
  4) echo "lose - restart same game"
     let ref_init=0
     aplay $RUN_DIR/sound/youlose.wav
     aplay $RUN_DIR/sound/losemusic.wav
     sleep 3
     aplay $RUN_DIR/sound/replay.wav
     continue
         ;;
  *) echo "return error!"
     exit
         ;;
esac

done

