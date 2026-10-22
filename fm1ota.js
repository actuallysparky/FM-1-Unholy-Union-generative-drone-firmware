// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
//
// The FM-1 USB-MIDI update protocol over WebMIDI (after tools/fm1_install.py).
// Step 1: the running firmware (official
// or Felucca) reads parts of the package and stages the update loader. Step 2:
// the loader reads the whole image and writes it. Both steps are "the device
// asks, we answer": cmd 0x30 read requests on the logical image.

const HS_QUERY = [0xF0, 0x00, 0x32, 0x45, 0x00, 0x00, 0x00, 0x40, 0x7F, 0xF7];
const UPGRADE = [0xF0, 0x22, 0x24, 0x35, 0x7F, 0xF7];
const MAXDATA = 512;
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const IS_OTA = (id) => /^ota-/i.test(id.model);

// errors carry a code the pages translate: notfound, model, stopped, noloader, lost, noreturn,
// mismatch (detail: the identity the device reports), badreq
function fail(code, msg, detail = "") { const e = new Error(msg); e.code = code; e.detail = detail; return e; }

// Recovery advice describes the last host-observed stage, not guaranteed flash contents.
export function updateFailure(stage,code,message,detail='') {
 const guidance=stage===1
  ? 'Update entry was requested. Loader or boot metadata may already have been written; this session did not start the bulk application transfer. Preserve the log and identify the current USB state before retrying. Retry only if running firmware or the MIDI update loader is reachable; otherwise use device-specific ROM recovery.'
  : stage===2
  ? 'Firmware writes may be incomplete. Keep power stable and preserve the log. Retry the reviewed package only if the MIDI update loader is reachable; otherwise use device-specific ROM recovery. A retry restarts the loader transfer.'
  : 'The transfer finished, but the requested firmware has not been verified. Do not report success. Preserve the log and check the returned identity/USB state before another update. If the MIDI loader is unavailable, use device-specific ROM recovery.';
 return fail(code,`${message} ${guidance}`,detail);
}

export function pack7(data) {
  const out = []; let acc = 0, nb = 0;
  for (const b of data) {
    acc |= b << nb; nb += 8;
    while (nb >= 7) { out.push(acc & 0x7F); acc >>>= 7; nb -= 7; }
  }
  if (nb) out.push(acc & 0x7F);
  return out;
}

export function unpack7(s) {
  const out = []; let acc = 0, nb = 0;
  for (const b of s) {
    acc |= (b & 0x7F) << nb; nb += 7;
    while (nb >= 8) { out.push(acc & 0xFF); acc >>>= 8; nb -= 8; }
  }
  return out;
}

function response(addr, data, fl = 0) {
  const len = data.length;
  const body = [0x00, 0x59, 0x30, (len + 8) & 0xFF, ((len + 8) >> 8) & 0xFF, 0, fl,
    addr & 0xFF, (addr >>> 8) & 0xFF, (addr >>> 16) & 0xFF, (addr >>> 24) & 0xFF,
    len & 0xFF, (len >> 8) & 0xFF, 0, ...data];
  let s = 0;
  for (let i = 6; i < body.length; i++) s += body[i];
  body.push(~s & 0xFF);
  return [0xF0, ...pack7(body), 0xF7];
}

function parseRequest(pkt) {
  if (pkt[0] !== 0xF0 || pkt[pkt.length - 1] !== 0xF7) return null;
  const u = unpack7(pkt.slice(1, -1));
  if (u.length !== 15 || u[0] !== 0 || u[1] !== 0x59 || u[2] !== 0x30) return null;
  let s = 0;
  for (let i = 6; i < 14; i++) s += u[i];
  if ((~s & 0xFF) !== u[14]) return null;
  return { fl: u[6], addr: (u[7] | (u[8] << 8) | (u[9] << 16) | (u[10] << 24)) >>> 0, len: u[11] | (u[12] << 8) | (u[13] << 16) };
}

function parseIdentity(pkt) {
  const d = unpack7(pkt.slice(1, -1));
  if (d.length !== 34 || d[0] !== 0 || d[1] !== 0x59 || d[2] !== 0x11) return null;
  const txt = String.fromCharCode(...d.slice(6, 33)).replace(/\0+$/, "");
  const m = /^([^_]+)_(\d+)$/.exec(txt);
  return m ? { text: txt, model: m[1], version: parseInt(m[2], 10) } : null;
}

// one MIDI in/out pair with a SysEx queue
class Link {
  constructor(input, output) {
    this.input = input; this.output = output; this.q = []; this.waiter = null;
    input.onmidimessage = (ev) => {
      if (ev.data[0] !== 0xF0) return;
      this.q.push(Array.from(ev.data));
      if (this.waiter) { const w = this.waiter; this.waiter = null; w(); }
    };
  }
  get lost() { return this.input.state === "disconnected" || this.output.state === "disconnected"; }
  // false when the port is gone (unplugged): WebMIDI throws on a disconnected output
  send(bytes) {
    try { this.output.send(bytes); return true; } catch (_) { return false; }
  }
  drain() { this.q.length = 0; }
  async read(ms) {
    if (!this.q.length) {
      let tm;
      await Promise.race([new Promise((r) => { this.waiter = r; }), new Promise((r) => { tm = setTimeout(r, ms); })]);
      clearTimeout(tm);
      this.waiter = null;
    }
    return this.q.shift() || null;
  }
  close() { this.input.onmidimessage = null; }
}

async function handshake(link, tries = 3) {
  link.drain();
  for (let t = 0; t < tries; t++) {
    if (!link.send(HS_QUERY)) return null;
    const end = Date.now() + 1000;
    while (Date.now() < end) {
      const p = await link.read(end - Date.now());
      if (!p) break;
      const id = parseIdentity(p);
      if (id) return id;
    }
  }
  return null;
}

export function transferLog(phase,detail,progress){
 if(progress?.sentBytes!==undefined){
  const coverage=progress.coveredBytes!==undefined?`${Math.round(progress.coveredBytes/1024)} of ${Math.round(progress.totalBytes/1024)} KiB supplied · `:'';
  return `${phase} / ${coverage}${Math.round(progress.sentBytes/1024)} KiB transferred (includes rereads)${progress.wiping?' · erasing user data':''}`;
 }
 return `${phase}${detail!==undefined?' / '+detail:''}`;
}

export class Updater {
  constructor(access, log) {
    this.access = access;
    this.log = log || (() => {});
    this.lastFailure = null;
  }

  // every in/out pair with the same port name that answers the handshake
  async find(filter) {
    const outs = [...this.access.outputs.values()];
    for (const input of this.access.inputs.values()) {
      if (input.state === "disconnected") continue;
      if (!/fm-1|blessed|felucca|ota|composite|sinco|usb-midi/i.test(input.name || "")) continue;   // never probe other gear
      const output = outs.find((o) => o.name === input.name && o.state !== "disconnected");
      if (!output) continue;
      try { await input.open(); await output.open(); } catch (_) { continue; }
      const link = new Link(input, output);
      const id = await handshake(link, 2);
      if (id && (!filter || filter(id))) return { link, id, name: input.name };
      link.close();
    }
    return null;
  }

  async waitFor(filter, ms) {
    const end = Date.now() + ms;
    while (Date.now() < end) {
      const dev = await this.find(filter);
      if (dev) return dev;
      await sleep(1000);
    }
    return null;
  }

  // serve read requests until the device asks for one of the finish addresses; stops early
  // when the port disappears (lost: true) instead of waiting out idleMs
  async serve(link, image, finishAddr, idleMs, onProgress, options = {}) {
    let served = 0, sentBytes = 0, coveredBytes = 0, last = Date.now();
    const supplied = new Uint8Array(image.length);
    for (;;) {
      const pkt = await link.read(1000);
      if (!pkt) {
        if (link.lost) return { served, finished: false, lost: true };
        if (Date.now() - last > idleMs) return { served, finished: false };
        continue;
      }
      const r = parseRequest(pkt);
      if (!r) continue;
      last = Date.now();
      if (r.addr === 0xE0000000 || r.addr === 0xF0000000) {
        if(r.len!==8 && !(r.addr===0xF0000000 && r.len===16)) throw fail("badreq","Invalid finish request length.");
        const answer=[...new TextEncoder().encode("success"),0];
        if(r.len===16) {
          answer.push(...(options.wipeUserData?[...new TextEncoder().encode("WIPEALL"),0]:new Array(8).fill(0)));
          if(options.wipeUserData && onProgress) onProgress(served,r.addr,{sentBytes,coveredBytes,totalBytes:image.length,wiping:true});
        }
        if(!link.send(response(r.addr,answer)))return {served,finished:false,lost:true};
        if (r.addr === finishAddr && r.len===8) return { served, finished: true };
        continue;
      }
      if (r.len > MAXDATA || r.addr + r.len > image.length) throw fail("badreq", `bad request ${r.addr.toString(16)}+${r.len}`);
      await sleep(10);
      if (!link.send(response(r.addr, Array.from(image.subarray(r.addr, r.addr + r.len)), r.fl))) {
        return { served, finished: false, lost: true };
      }
      served++;sentBytes+=r.len;
      // The loader verifies and rereads overlapping/out-of-order ranges. Count
      // each successfully supplied image byte once, independently of traffic.
      for(let i=r.addr;i<r.addr+r.len;i++)if(!supplied[i]){supplied[i]=1;coveredBytes++;}
      if (onProgress) onProgress(served, r.addr, {sentBytes,coveredBytes,totalBytes:image.length});
    }
  }

  // full install: running firmware -> loader -> new firmware
  async install(image, product, onStep, options = {}) {
    const step = onStep || (() => {});
    const dev = await this.find((id) => !IS_OTA(id));
    if (!dev) throw fail("notfound", "FM-1 not found. Check the data cable and MIDI contention. No update command was sent by this attempt.");
    const [model] = product.split("_");
    if (dev.id.model !== model) { dev.link.close(); throw fail("model", `the device is ${dev.id.text}, the package is for ${product}. No update command was sent by this attempt.`); }
    step("start", dev.id.text);
    dev.link.send(UPGRADE);
    await sleep(2000);
    let s1;
    try { s1 = await this.serve(dev.link, image, 0xE0000000, 8000, (n,addr,progress) => step("verify", n, progress)); }
    catch(e) { throw updateFailure(1,e.code || "error",e.message,e.detail); }
    finally { dev.link.close(); }
    if (!s1.finished) throw updateFailure(1,s1.lost ? "lost" : "stopped", `The device ${s1.lost ? "was disconnected" : "stopped answering"} after ${s1.served} requests.`);
    step("loader");
    await sleep(3000);
    const ota = await this.waitFor((id) => id.model === "ota-" + model, 30000);
    if (!ota) throw updateFailure(1,"noloader", "The MIDI update loader did not appear.");
    const s2 = await this.write(ota, image, step, options);
    if (!s2.finished) throw updateFailure(2,s2.lost ? "lost" : "stopped", `The loader ${s2.lost ? "was disconnected" : "stopped answering"} after ${s2.served} requests.`);
    step("reboot");
    await sleep(3000);
    const back = await this.waitFor((id) => !IS_OTA(id), 40000);
    if (!back) throw updateFailure(3,"noreturn", "The FM-1 did not return after the transfer.");
    back.link.close();
    if (back.id.text !== product) throw updateFailure(3,"mismatch", `The device reports ${back.id.text}; expected ${product}.`, back.id.text);
    step("done", back.id.text);
    return back.id.text;
  }

  // step 2 with the loader found by find()/waitFor()
  async write(ota, image, step, options = {}) {
    if(options.wipeUserData && ota.id.text!=="ota-FM-1_901") {ota.link.close();throw fail("wipeunsupported","This update loader cannot erase user data. Resume without the wipe option, then install a wipe-capable offering.");}
    ota.link.send(UPGRADE);
    await sleep(2000);
    let s2;
    try { s2 = await this.serve(ota.link, image, 0xF0000000, 180000, (n,addr,progress) => step(progress?.wiping?"wipe":"write", n, progress), options); }
    catch(e) { throw updateFailure(2,e.code || "error",e.message,e.detail); }
    finally { ota.link.close(); }
    return s2;
  }

  // resume: the device is already in update mode (loader) -> true when the write finished
  async resume(image, onStep, product = null, options = {}) {
    this.lastFailure = null;
    const model = product && product.split("_")[0];
    const ota = await this.find((id) => model ? id.model === "ota-" + model : IS_OTA(id));
    if (!ota) { this.lastFailure=updateFailure(2,"noloader","No MIDI update loader answered; the previous session's flash state is unknown. No new update command was sent by this attempt."); return false; }
    const step = onStep || (() => {});
    const result = await this.write(ota, image, step, options);
    if (!result.finished) { this.lastFailure=updateFailure(2,result.lost ? "lost" : "stopped",`The loader ${result.lost ? "was disconnected" : "stopped answering"} after ${result.served} requests.`); return false; }
    if (product) {
      step("reboot");
      await sleep(3000);
      const back = await this.waitFor((id) => id.model === model, 40000);
      if (!back) throw updateFailure(3,"noreturn", "The FM-1 did not return after resuming the write.");
      back.link.close();
      if (back.id.text !== product) throw updateFailure(3,"mismatch", `The device reports ${back.id.text}; expected ${product} after resuming.`, back.id.text);
      step("done", back.id.text);
    }
    return true;
  }
}
