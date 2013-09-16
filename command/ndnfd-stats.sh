#!/bin/bash
CCNDID=`ccnpeek ccnx:/%C1.M.S.localhost/%C1.M.SRV/ccnd | ccnbx -d - PublisherPublicKeyDigest | php -r 'echo rawurlencode(file_get_contents("php://stdin"));'`
URI=ccnx:/ccnx/$CCNDID/ndnfd-stats
ccnpeek -c $URI
