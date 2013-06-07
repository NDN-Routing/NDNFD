var http = require('http');

var dispatch = [
  [/^\/stats\.xml$/, require('./stats').serve_xml]
];

var server = http.createServer().listen(9696);
server.on('request', function(req, resp){
  var served = dispatch.some(function(tuple){
    if (!tuple[0].test(req.url)) return false;
    tuple[1](req, resp);
    return true;
  });
  if (!served) {
    resp.writeHead(404);
    resp.end();
  }
});
