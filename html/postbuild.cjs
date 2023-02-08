const exec = require("child_process").exec;
const platform = require("os").platform();
const isWindows = /^win/.test(platform);
const cmd = isWindows
  ? `${process.cwd()}\\postbuild.bat`
  : `${process.cwd()}/postbuild.sh`;
exec(cmd);