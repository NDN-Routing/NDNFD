# NDNFD web server

NDNFD web server is a Node.js v0.10.x application that provides NDNFD related services over HTTP.

To start the web server, type `node ./index.js`

## status page

NDNFD router status is located at `http://127.0.0.1:9696/stats.xml` as an XML document.

## other potential services

The web server could host a WebSockets endpoint for NDN-JS connectivity, and have other diagnostics and monitoring utilities.

