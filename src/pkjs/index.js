// PebbleKit JS: reicht ausschliesslich die Clay-Einstellungen an die Uhr
// weiter. Zeit und Datum liest die Uhr selbst.

var Clay = require('@rebble/clay');
var clayConfig = require('./config');

var clay = new Clay(clayConfig);
