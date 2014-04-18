#!/bin/sh
EZ_BIN=$EZ/bin
export EZ_BIN
EZ_SERVICES=$EZ/conf/services
export EZ_SERVICES
EZ_NODES=$EZ/conf/nodes
export EZ_NODES
EZ_MONITOR=$EZ/conf/monitor
export EZ_MONITOR
EZ_LOG=$EZ/log
export EZ_LOG
alias ez_svc_status="echo =\> Querying OpenHA services status [$EZ_BIN/service -s];$EZ_BIN/service -s"
alias ez_hb_status="echo =\> Querying OpenHA heartbeat status [$EZ_BIN/hb -s];$EZ_BIN/hb -s"
alias ez_ps="ps auxww|egrep \"[h]eartd|[h]eartc|[n]mond\""
