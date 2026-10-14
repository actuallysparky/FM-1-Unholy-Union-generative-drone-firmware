// SPDX-License-Identifier: GPL-3.0-only
// Single bundled package verification. No MIDI, updater or installation action.
import {Package,logicalImage,sha256hex} from './fm1pkg.js';
export async function verifyBundledPackage(bytes,expected) {
 if(bytes.length!==expected.bytes)throw Error('Bundled firmware size mismatch. Reload or repair the local preview.');
 const sha256=await sha256hex(bytes);
 if(sha256!==expected.sha256)throw Error('Bundled firmware checksum mismatch. Installation is blocked.');
 const pkg=new Package(bytes);
 if(pkg.product!==expected.product)throw Error('Bundled firmware identity mismatch. Installation is blocked.');
 return {product:pkg.product,sha256,bytes:bytes.length,image:logicalImage(bytes)};
}
export async function fetchBundledPackage(expected,fetcher=fetch) {
 let response;
 try{response=await fetcher(expected.filename,{cache:'no-store'});}catch{throw Error('Bundled firmware could not be fetched. Check the local preview server, then retry. Installation is blocked.');}
 if(!response.ok)throw Error(`Bundled firmware could not load (HTTP ${response.status}). Repair the local preview before installing.`);
 return verifyBundledPackage(new Uint8Array(await response.arrayBuffer()),expected);
}
