
var Module = (function() {
  var _scriptDir = typeof document !== 'undefined' && document.currentScript ? document.currentScript.src : undefined;
  return (
function(Module) {
  Module = Module || {};






  return Module;
}
);
})();
if (typeof exports === 'object' && typeof module === 'object')
    module.exports = Module;
  else if (typeof define === 'function' && define['amd'])
    define([], function() { return Module; });
  else if (typeof exports === 'object')
    exports["Module"] = Module;
  