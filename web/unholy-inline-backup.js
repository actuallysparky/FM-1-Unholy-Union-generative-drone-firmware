// SPDX-License-Identifier: GPL-3.0-only
import {BackupConnection,captureBackup} from './fm1backup.js';
export async function gatherScriptures(access,firmware,download,onProgress=()=>{}) {
 const inputs=[...access.inputs.values()].filter(p=>p.state!=='disconnected'&&/fm-1|blessed|felucca|unholy|composite|sinco|usb-midi/i.test(p.name||''));
 if(inputs.length!==1)throw Error('Connect exactly one FM-1 before gathering its scriptures.');
 const input=inputs[0],outputs=[...access.outputs.values()].filter(p=>p.state!=='disconnected'&&p.name===input.name);
 if(outputs.length!==1)throw Error('A single matching FM-1 output is required.');
 const output=outputs[0];await input.open();await output.open();
 const previous=input.onmidimessage,connection=new BackupConnection(input,output);
 try {
  const archive=await captureBackup((request,options)=>connection.request(request,options),firmware,onProgress);
  await download(archive);return archive;
 }finally{connection.close();input.onmidimessage=previous;}
}
export function downloadScriptures(archive) {
 const url=URL.createObjectURL(new Blob([JSON.stringify(archive,null,2)],{type:'application/json'}));
 const anchor=document.createElement('a');anchor.href=url;anchor.download=`UnholyUnion-scriptures-${new Date().toISOString().slice(0,10)}.json`;
 document.body.append(anchor);try{anchor.click();}finally{anchor.remove();setTimeout(()=>URL.revokeObjectURL(url),1000);}
}
