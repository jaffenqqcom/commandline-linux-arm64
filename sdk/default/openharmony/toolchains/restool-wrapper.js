#!/usr/bin/env node
const fs = require('fs');
const path = require('path');
const args = process.argv.slice(2);
let rc = null;
for (let i = 0; i < args.length; i++) { if (args[i] === '-l' && i+1 < args.length) { rc = args[i+1]; break; } }
if (!rc) { console.error('[restool] No config'); process.exit(1); }
const c = JSON.parse(fs.readFileSync(rc, 'utf-8'));
const out = c.output;
function cp(s,d) {
  if (!fs.existsSync(s)) return;
  fs.mkdirSync(path.dirname(d), { recursive: true });
  fs.copyFileSync(s, d);
}
if (out) {
  // Copy app resources
  if (c.applicationResource) {
    function cpDir(s,d) {
      if (!fs.existsSync(s)) return;
      fs.mkdirSync(d, { recursive: true });
      for (const e of fs.readdirSync(s, { withFileTypes: true })) {
        const sp = path.join(s, e.name), dp = path.join(d, e.name);
        e.isDirectory() ? cpDir(sp, dp) : (fs.mkdirSync(path.dirname(dp), { recursive: true }), fs.copyFileSync(sp, dp));
      }
    }
    cpDir(c.applicationResource, path.join(out, 'resources', 'app'));
  }
  // Copy module resources
  if (c.moduleResources) {
    for (const r of c.moduleResources) {
      const b = path.join(r, 'base');
      if (fs.existsSync(b)) cpDir(b, path.join(out, 'resources', 'base'));
    }
  }
  // Copy module.json from configPath
  if (c.configPath && fs.existsSync(c.configPath)) {
    cp(c.configPath, path.join(out, 'module.json'));
  }
  // Generate resource.index
  const idx = path.join(out, 'resource.index');
  const files = [];
  function walk(d,pfx) {
    if (!fs.existsSync(d)) return;
    for (const e of fs.readdirSync(d, { withFileTypes: true })) {
      const fp = path.join(d, e.name);
      e.isDirectory() ? walk(fp, pfx ? pfx+'/'+e.name : e.name) : files.push(pfx ? pfx+'/'+e.name : e.name);
    }
  }
  walk(out, '');
  fs.writeFileSync(idx, files.join('\n') || '\n');
  // Generate binary resources.index (for ohos-bundletool validation)
  try {
    var bIdx = path.join(out, 'resources.index');
    var bf = [];
    function walkBin(d,p) {
      if (!fs.existsSync(d)) return;
      for (var e of fs.readdirSync(d, { withFileTypes: true })) {
        var fp = path.join(d, e.name);
        if (e.isDirectory()) walkBin(fp, p ? p+'/'+e.name : e.name);
        else if (!['resConfig.json','resources.index','resource.index','opt-compression.json'].includes(e.name)) bf.push(p ? p+'/'+e.name : e.name);
      }
    }
    walkBin(out, '');
    var buf = Buffer.alloc(16 + bf.length * 16);
    buf.writeUInt32LE(5, 0);  // magic
    buf.writeUInt32LE(1, 4);  // version
    buf.writeUInt32LE(bf.length, 8);  // file count
    buf.writeUInt32LE(0, 12);  // key count
    var offset = 16;
    for (var f of bf) {
      var nb = Buffer.from(f, 'utf-8');
      var tmp = Buffer.alloc(4 + nb.length + 8);
      tmp.writeUInt32LE(nb.length, 0);
      nb.copy(tmp, 4);
      var fsize = 0;
      try { fsize = fs.statSync(path.join(out, f)).size; } catch(e) {}
      tmp.writeBigUInt64LE(BigInt(fsize), 4 + nb.length);
      buf = Buffer.concat([buf, tmp]);
    }
    fs.writeFileSync(bIdx, buf);
  } catch(e) { console.error('[restool] binary index error:', e.message); }

  // Generate ResourceTable files
  const gd = c.ResourceTable ? path.dirname(c.ResourceTable[0]) : null;
  if (gd) {
    fs.mkdirSync(gd, { recursive: true });
    if (c.ResourceTable) {
      for (const rt of c.ResourceTable) {
        if (!fs.existsSync(rt)) fs.writeFileSync(rt, path.extname(rt)==='.h' ? '// Auto-generated\n' : 'export const ResourceTable = {};\n');
      }
    }
  }
  // Create ids map
  if (c.ids) { fs.mkdirSync(c.ids, { recursive: true }); if (!fs.existsSync(path.join(c.ids, 'id_defined.json'))) fs.writeFileSync(path.join(c.ids, 'id_defined.json'), '{}'); }
  console.log('[restool] Processed', out);
}
process.exit(0);
