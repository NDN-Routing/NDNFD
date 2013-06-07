var child_process = require('child_process');

function serve_xml(req, resp) {
  resp.setHeader('Content-Type', 'application/xml');
  var proc = child_process.exec('ccnpeek -c ccnx:/ccnx/`ccnpeek ccnx:/%C1.M.S.localhost/%C1.M.SRV/ccnd | ccnbx -d - PublisherPublicKeyDigest | php -r \'echo urlencode(file_get_contents("php://stdin"));\'`/ndnfd-stats');
  proc.stderr.on('data', function(){ resp.removeHeader('Content-Type'); resp.writeHead(500); resp.end(); });
  proc.stdout.on('data', function(data){ resp.write(data); });
  proc.on('close', function(){ resp.end(); });
}

exports.serve_xml = serve_xml;
