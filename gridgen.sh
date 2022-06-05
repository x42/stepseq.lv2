#!/usr/bin/env bash
NOTES=$1
STEPS=$2

if test -z "$NOTES" -o -z "$STEPS"; then
	echo "Number of notes and steps must be given."
	exit 1
fi

if ! [ "$NOTES" -eq "$NOTES" ] 2>/dev/null; then
	echo "Number of Notes must be an integer"
	exit 1
fi

if ! [ "$STEPS" -eq "$STEPS" ] 2>/dev/null; then
	echo "Number of Steps must be an integer"
	exit 1
fi


IDX=11

if test -n "$MOD"; then
mkdir -p modgui
	MODICON=modgui/icon-stepseq.html
	MODSTYLE=modgui/style-stepseq.css
else
	MODICON=/dev/null
	MODSTYLE=/dev/null
fi

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

for ((n=1; n <= $NOTES; n++)); do
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

for ((n=1; n <= $NOTES; n++)); do
	echo '<tr><th><div class="mod-knob-16seg-image note" mod-role="input-control-port" mod-port-symbol="note'$n'" x42-role="seq-note"></div></th>' >> $MODICON
	for ((s=1; s <= $STEPS; s++)); do

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
		echo '<td><div class="togglebtn on" grid-col="'$s'" grid-row="'$n'" mod-widget="switch" mod-role="input-control-port" mod-port-symbol="grid_'$s'_'$n'">'$s'</div></td>' >> $MODICON
	  IDX=$(($IDX + 1))
	done
	echo '<td><div class="resetbutton row" grid-row="'$n'" title="Clear Note Row">C</div></td>' >> $MODICON
	echo '</tr>' >> $MODICON
done

if test -z "$MOD"; then
	exit
fi

echo '<tr><th></th>' >> $MODICON
for ((s=1; s <= $STEPS; s++)); do
	echo '<td><div class="resetbutton col" grid-col="'$s'" title="Clear Column Step:'$s'">C</div></td>' >> $MODICON
done
echo '<td><div class="resetbutton all" title="Clear Grid">C</div></td>' >> $MODICON
echo '</tr>' >> $MODICON

cat misc/mod_icon.tail >> $MODICON

WIDTH=$(( 250 + $STEPS * 46 ))
HEIGHT=$(( 192 + $NOTES * 46 ))

if test -f misc/box_s${STEPS}_n${NOTES}.png; then
	cp misc/box_s${STEPS}_n${NOTES}.png modgui/box.png
elif test -x misc/boxmaker; then
	misc/boxmaker modgui/box.png $STEPS $NOTES
else
	echo "*** NO BOX BACKGROUND FOR $STEPS steps, $NOTES notes" >&2
	echo "*** compile misc/boxmaker for this build-host" >&2
	echo "*** make -C misc boxmaker" >&2
	exit 1
fi

sed "s/@WIDTH@/$WIDTH/g;s/@HEIGHT@/$HEIGHT/g" misc/style.css.in > $MODSTYLE
