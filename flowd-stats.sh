#!/bin/sh

FR=/usr/local/bin/flowd-reader

TOTAL_FLOWS=0
TOTAL_BYTES=0
HTTP_FLOWS=0
HTTP_BYTES=0
HTTPS_FLOWS=0
HTTPS_BYTES=0
QUIC_FLOWS=0
QUIC_BYTES=0

for i in `${FR} ${1} | awk '{ print $NF ; }'` ; do
	TOTAL_BYTES=$((${TOTAL_BYTES}+${i}));
	TOTAL_FLOWS=$((${TOTAL_FLOWS}+1));
done

for i in `${FR} ${1} | grep 'proto 6' | grep ':80 ' | awk '{ print $NF ; }'` ; do
	HTTP_BYTES=$((${HTTP_BYTES}+${i}));
	HTTP_FLOWS=$((${HTTP_FLOWS}+1));
done

for i in `${FR} ${1} | grep 'proto 6' | grep ':443 ' | awk '{ print $NF ; }'` ; do
	HTTPS_BYTES=$((${HTTPS_BYTES}+${i}));
	HTTPS_FLOWS=$((${HTTPS_FLOWS}+1));
done

for i in `${FR} ${1} | grep 'proto 17' | grep ':443 ' | awk '{ print $NF ; }'` ; do
	QUIC_BYTES=$((${QUIC_BYTES}+${i}));
	QUIC_FLOWS=$((${QUIC_FLOWS}+1));
done

SUM_FLOWS=$((${HTTP_FLOWS} + ${HTTPS_FLOWS} + ${QUIC_FLOWS}))
SUM_BYTES=$((${HTTP_BYTES} + ${HTTPS_BYTES} + ${QUIC_BYTES}))
OTHER_FLOWS=$((${TOTAL_FLOWS} - ${SUM_FLOWS}))
OTHER_BYTES=$((${TOTAL_BYTES} - ${SUM_BYTES}))

HTTP_FLOWS_SECT=$(echo "(${HTTP_FLOWS} / ${TOTAL_FLOWS}) * 360" | bc -l)
HTTPS_FLOWS_SECT=$(echo "(${HTTPS_FLOWS} / ${TOTAL_FLOWS}) * 360" | bc -l)
QUIC_FLOWS_SECT=$(echo "(${QUIC_FLOWS} / ${TOTAL_FLOWS}) * 360" | bc -l)
OTHER_FLOWS_SECT=$(echo "(${OTHER_FLOWS} / ${TOTAL_FLOWS}) * 360" | bc -l)

HTTP_BYTES_SECT=$(echo "(${HTTP_BYTES} / ${TOTAL_BYTES}) * 360" | bc -l)
HTTPS_BYTES_SECT=$(echo "(${HTTPS_BYTES} / ${TOTAL_BYTES}) * 360" | bc -l)
QUIC_BYTES_SECT=$(echo "(${QUIC_BYTES} / ${TOTAL_BYTES}) * 360" | bc -l)
OTHER_BYTES_SECT=$(echo "(${OTHER_BYTES} / ${TOTAL_BYTES}) * 360" | bc -l)

echo "Total flows:\t${TOTAL_FLOWS}"
echo "HTTP flows:\t${HTTP_FLOWS}\t(${HTTP_FLOWS_SECT})"
echo "HTTPS flows:\t${HTTPS_FLOWS}\t(${HTTPS_FLOWS_SECT})"
echo "QUIC flows:\t${QUIC_FLOWS}\t(${QUIC_FLOWS_SECT})"
echo "Other flows:\t${OTHER_FLOWS}\t(${OTHER_FLOWS_SECT}"
echo ""
echo "Total bytes:\t${TOTAL_BYTES}"
echo "HTTP bytes:\t${HTTP_BYTES}\t(${HTTP_BYTES_SECT})"
echo "HTTPS bytes:\t${HTTPS_BYTES}\t(${HTTPS_BYTES_SECT})"
echo "QUIC bytes:\t${QUIC_BYTES}\t(${QUIC_BYTES_SECT})"
echo "Other bytes:\t${OTHER_BYTES}\t(${OTHER_BYTES_SECT})"


PLOT_BYTES='plot-bytes.plot'
PLOT_FLOWS='plot-flows.plot'

echo "reset session" > ${PLOT_BYTES}
echo "set output \"bytes.png\"" >> ${PLOT_BYTES}
echo "set term png size 600, 340" >> ${PLOT_BYTES}
echo "unset border" >> ${PLOT_BYTES}
echo "unset xtics" >> ${PLOT_BYTES}
echo "unset ytics" >> ${PLOT_BYTES}
echo "set title 'bytes'" >> ${PLOT_BYTES}
echo '$Data <<EOD' >> ${PLOT_BYTES}
SECTOR_START=0
SECTOR_END=${HTTP_BYTES_SECT}
echo "0\t0\t5\t${SECTOR_START}\t${SECTOR_END}\t0xff0000 'HTTP'" >> ${PLOT_BYTES}
SECTOR_START=${SECTOR_END}
SECTOR_END=$(echo ${SECTOR_START} + ${HTTPS_BYTES_SECT} | bc -l)
echo "0\t0\t5\t${SECTOR_START}\t${SECTOR_END}\t0x00ff00 'HTTPS'" >> ${PLOT_BYTES}
SECTOR_START=${SECTOR_END}
SECTOR_END=$(echo ${SECTOR_END} + ${QUIC_BYTES_SECT} | bc -l)
echo "0\t0\t5\t${SECTOR_START}\t${SECTOR_END}\t0x0000ff 'QUIC'" >> ${PLOT_BYTES}
SECTOR_START=${SECTOR_END}
SECTOR_END=$(echo ${SECTOR_END} + ${OTHER_BYTES_SECT} | bc -l)
echo "0\t0\t5\t${SECTOR_START}\t${SECTOR_END}\t0xffff00 'OTHER'" >> ${PLOT_BYTES}
echo "EOD" >> ${PLOT_BYTES}

cat <<EOF >> ${PLOT_BYTES}
set xrange [-6:6]
set yrange [-6:6]
set size ratio -1
set style fill transparent solid 0.3
set key noautotitle
set angle degrees

plot \$Data u 1:2:3:4:5:6 w circles lc rgb var, \
        '' u (a=(\$4+\$5)/2, \$3*0.7*cos(a)):(\$3*0.7*sin(a)):7 w labels
### end of script
EOF

echo "reset session" > ${PLOT_FLOWS}
echo "set output \"flows.png\"" >> ${PLOT_FLOWS}
echo "set term png size 600, 340" >> ${PLOT_FLOWS}
echo "unset border" >> ${PLOT_FLOWS}
echo "unset xtics" >> ${PLOT_FLOWS}
echo "unset ytics" >> ${PLOT_FLOWS}
echo "set title 'flows'" >> ${PLOT_FLOWS}
echo '$Data <<EOD' >> ${PLOT_FLOWS}
SECTOR_START=0
SECTOR_END=${HTTP_FLOWS_SECT}
echo "0\t0\t5\t${SECTOR_START}\t${SECTOR_END}\t0xff0000 'HTTP'" >> ${PLOT_FLOWS}
SECTOR_START=${SECTOR_END}
SECTOR_END=$(echo ${SECTOR_START} + ${HTTPS_FLOWS_SECT} | bc -l)
echo "0\t0\t5\t${SECTOR_START}\t${SECTOR_END}\t0x00ff00 'HTTPS'" >> ${PLOT_FLOWS}
SECTOR_START=${SECTOR_END}
SECTOR_END=$(echo ${SECTOR_END} + ${QUIC_FLOWS_SECT} | bc -l)
echo "0\t0\t5\t${SECTOR_START}\t${SECTOR_END}\t0x0000ff 'QUIC'" >> ${PLOT_FLOWS}
SECTOR_START=${SECTOR_END}
SECTOR_END=$(echo ${SECTOR_END} + ${OTHER_FLOWS_SECT} | bc -l)
echo "0\t0\t5\t${SECTOR_START}\t${SECTOR_END}\t0xffff00 'OTHER'" >> ${PLOT_FLOWS}
echo "EOD" >> ${PLOT_FLOWS}

cat <<EOF >> ${PLOT_FLOWS}
set xrange [-6:6]
set yrange [-6:6]
set size ratio -1
set style fill transparent solid 0.3
set key noautotitle
set angle degrees

plot \$Data u 1:2:3:4:5:6 w circles lc rgb var, \
        '' u (a=(\$4+\$5)/2, \$3*0.7*cos(a)):(\$3*0.7*sin(a)):7 w labels
### end of script
EOF

gnuplot < ${PLOT_FLOWS}
gnuplot < ${PLOT_BYTES}
