#!/bin/bash
MYDB='mysql -N -u wwiiol -pfreebird -h 192.168.20.101 wwiionline -e '
USERID=`$MYDB "select playerid from wwii_player where callsign='"$1"';"`
CUSTID=`$MYDB "select customerid from wwii_player where callsign='"$1"';"`
echo "Processing god mode for user id: "$USERID

RATGROUP=`$MYDB "select group_oid from auth.chat_group_members where playerid="$USERID" and group_oid=1;"`
if [ -z "$RATGROUP" ]; then
   RESULT=`$MYDB "insert into auth.chat_group_members (group_oid, playerid) values (1,"$USERID");"`
   if [ ! -z "$RESULT" ]; then
       echo "Error adding user to chat_group_members: " $RESULT
       exit
   else
       echo "Added chat group entry for new rat"
   fi
else
   echo "chat_group_members: already rat"
fi

SUB=`$MYDB "select subscriptionID from auth.auth_subscribed where customerID="$CUSTID" and subscriptionID=188;"`
if [ -z "$SUB" ]; then
   RESULT=`$MYDB "insert into auth.auth_subscribed (customerID,subscriptionID, status) values ("$CUSTID", 188, 'active');"`
   if [ ! -z "$RESULT" ]; then
       echo "Error adding user to auth_subscribed: " $RESULT
       exit
   else
       echo "Added subscription for new RAT"
   fi
else
   RESULT=`$MYDB "update auth.auth_subscribed set status='active' where customerID="$CUSTID" and subscriptionID=188;"`
   if [ ! -z "$RESULT" ]; then
       echo "Error updating active state of auth_subscribed": $RESULT
   fi
   echo "auth_subscribed: updated to active"
fi

ARENA=`$MYDB "select arenaID from auth.auth_subarenaassoc where arenaID=250 and subscriptionID=188;"`
if [ -z "$ARENA" ]; then
   RESULT=`$MYDB "insert into auth.auth_subarenaassoc (subscriptionID,arenaID) values (188,250);"`
   if [ ! -z "$RESULT" ]; then
       echo "Error adding user to auth_subscribed: " $RESULT
       exit
   else
       echo "Added arena 250 to GOD type"
   fi
else
   echo "Arena 250 already attached to GOD type"
fi

