#!/bin/bash
NOTES=$1
STEPS=$2

MODICON=modgui/icon-stepseq.html
MODSTYLE=modgui/style-stepseq.css
IDX=8

function twelvetet {
	num=$(( $1 % 7 ))
	case $num in
		1) echo 1 ;;
		2) echo 3 ;;
		3) echo 5 ;;
		4) echo 6 ;;
		5) echo 8 ;;
		6) echo 10 ;;
		0) echo 11 ;;
	esac
}

for n in `seq 1 $NOTES`; do
	# TODO musical scale
	OCT=$(( ($n - 1) / 7 ))
	NOT=$(twelvetet $n)
	NN=$(( 70 - 12 * $OCT - $NOT ))
sed "s/@IDX@/$IDX/;s/@NOTE@/$n/g;s/@NN@/$NN/g" << EOF
	] , [
		a lv2:InputPort, lv2:ControlPort ;
		lv2:index @IDX@;
		lv2:symbol "note@NOTE@";
		lv2:name "Note @NOTE@";
		lv2:default @NN@;
		lv2:minimum 0;
		lv2:maximum 127;
		lv2:portProperty lv2:integer
EOF
	IDX=$(($IDX + 1))
done

cat misc/mod_icon.head > $MODICON

for n in `seq 1 $NOTES`; do
	echo '<tr><th><div class="mod-knob-note-image" mod-role="input-control-port" mod-port-symbol="note'$n'"></div></th>' >> $MODICON
	for s in `seq 1 $STEPS`; do

		sed "s/@IDX@/$IDX/;s/@NOTE@/$n/g;s/@STEP@/$s/g" << EOF
	] , [
		a lv2:InputPort, lv2:ControlPort ;
		lv2:index @IDX@ ;
		lv2:symbol "grid_@STEP@_@NOTE@" ;
		lv2:name "Grid S: @STEP@ N: @NOTE@";
		lv2:default 0 ;
		lv2:minimum 0 ;
		lv2:maximum 127 ;
		lv2:portProperty lv2:integer;
EOF
		echo '<td><div class="togglebtn on"  mod-widget="switch" mod-role="input-control-port" mod-port-symbol="grid_'$s'_'$n'">'$s'</div></td>' >> $MODICON
	  IDX=$(($IDX + 1))
	done
	echo '</tr>' >> $MODICON
done

cat misc/mod_icon.tail >> $MODICON

WIDTH=$(( 172 + $STEPS * 46 ))
HEIGHT=$(( 160 + $NOTES * 50 ))

sed "s/@WIDTH@/$WIDTH/g;s/@HEIGHT@/$HEIGHT/g" misc/style.css.in > $MODSTYLE
