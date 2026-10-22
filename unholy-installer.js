// SPDX-License-Identifier: GPL-3.0-only
import {offering} from './unholy-offering.js?v=v15-e5f2813f7ce5';
import {InstallerState} from './unholy-controller.js?v=v15-e5f2813f7ce5';
import {fetchBundledPackage} from './unholy-bundled-package.js?v=v15-e5f2813f7ce5';
import {Updater,transferLog} from './fm1ota.js?v=v15-e5f2813f7ce5';
const $=id=>document.getElementById(id);
const demo=new URLSearchParams(location.search).get('demo')==='1';
const experimental=!offering.release.targetValidated;
let release=demo?{...offering.release,available:true,targetValidated:true,sha256:'SIMULATION',bytes:1}:offering.release;
let state=new InstallerState(release,{simulation:demo,experimental:!demo&&experimental});
let updater=null,access=null,payload=null,bundled=null,loading=false,loadGeneration=0,backingUp=false,connecting=false;
const matchingInputs=access=>[...access.inputs.values()].filter(p=>p.state!=='disconnected'&&/fm-1|blessed|felucca|unholy|ota|composite|sinco|usb-midi/i.test(p.name||''));
const stockStartingIdentity=()=>state.device?.model==='FM-1'&&['FM-1_014','FM-1_015'].includes(state.device.text);
const stockBackupNotice='This page cannot export a complete backup from stock firmware. Keep your original voice-bank files before switching.';
const label={idle:'Awaiting instrument',connected:'Instrument identified',incompatible:'Firmware not supported',validated:'Package checked',starting:'Entering update mode',verify:'Loader preparation',loader:'Waiting for update loader',write:'Writing firmware',wipe:'Cleansing user data',reboot:'Waiting for identity',done:demo?'SIMULATED completion':'Identity verified',error:'Update error',disconnected:'Connection lost',cancelled:'Session cancelled'};
function log(s){const box=$('log');box.textContent=`${new Date().toLocaleTimeString()}  ${s}\n`+box.textContent;box.scrollTop=0;}
function render(){
 $('phase').textContent=label[state.phase]||state.phase;
 $('device').textContent=state.device?`${demo?'SIMULATED ':''}${state.device.text}`:'No device connected.';
 $('install').disabled=connecting||backingUp||!state.ready;$('install').textContent=demo?'Simulate install':'Install Apocryphal';
 $('connect').disabled=connecting||state.busy||backingUp;$('connect').hidden=!!state.device;$('cancel').disabled=state.busy||backingUp;$('cancel').hidden=!state.device;
 $('download-backup').disabled=demo||connecting||state.busy||backingUp||!state.compatible||!access||stockStartingIdentity();
 if(stockStartingIdentity())$('backup-status').textContent=stockBackupNotice;
 else if($('backup-status').textContent===stockBackupNotice)$('backup-status').textContent='Optional. Connect your FM-1 to download a backup.';
 $('install-notes').textContent=stockStartingIdentity()?'Stock presets and sequences are not converted into UnholyUnion projects. Keep USB and power connected throughout the rite.':'Your saved UnholyUnion music stays with you. Keep USB and power connected throughout the rite.';
 $('retry-package').disabled=state.busy||loading;
 $('progress').hidden=!state.busy;
 $('status').textContent=state.error|| (state.phase==='done'?(demo?'Simulation completed. No device was contacted.':`Device returned and reports ${release.product}. Press PLAY to pray.`):state.busy?'Transfer active. Keep USB and power stable. Supplied data counts each package byte once; transferred data includes rereads. These counts do not measure flash-write progress.':demo?'Simulation room: all events below are mocks.':'No update has been initiated.');
 if(!demo&&!state.busy&&state.device&&!state.compatible)$('status').textContent=`Detected ${state.device.text}. This offering has no approved installation path from that identity. No update has been started.`;
}
$('connect').addEventListener('click',async()=>{
 if(connecting||backingUp||state.busy)return;connecting=true;render();
 try {
 state.wipeUserData=false;
 if(demo){state.connect({model:'FM-1',text:'FM-1_SIMULATED'});state.load({product:release.product,sha256:'SIMULATION',bytes:1});log('SIMULATED device and package. No MIDI access.');render();return;}
 if(!isSecureContext||!navigator.requestMIDIAccess)throw Error('A secure desktop Chrome/Edge page with Web MIDI is required. Firefox and Safari are unsupported.');
 $('connect').disabled=true;$('status').textContent='Requesting MIDI SysEx permission, then checking identity…';
 access=await navigator.requestMIDIAccess({sysex:true});if(!access.sysexEnabled)throw Error('SysEx permission was not granted.');
 if(matchingInputs(access).length!==1)throw Error('Connect exactly one matching FM-1 MIDI input. Ambiguous device sets are refused.');
 updater=new Updater(access,log);const found=await updater.find();if(!found)throw Error('No supported FM-1 identity answered. Check USB, MIDI permissions, and other apps.');
 found.link.close();state.connect(found.id);payload=null;authorizeBundled();log(`Read-only identity: ${found.id.text}`);
 access.onstatechange=ev=>{if(ev.port.state==='disconnected'&&!state.busy){state.disconnected();payload=null;render();}};
 }catch(e){state.fail(e);log(e.message);}finally{connecting=false;render();}
});

$('download-backup').addEventListener('click',async()=>{
 if(backingUp||state.busy||demo||!access||!state.compatible||stockStartingIdentity())return;
 backingUp=true;render();$('backup-status').textContent='Gathering your scriptures. Keep playback stopped and your FM-1 connected…';
 try{
  const {gatherScriptures,downloadScriptures}=await import('./unholy-inline-backup.js');
  await gatherScriptures(access,state.device.text,downloadScriptures,(done,total)=>{$('backup-status').textContent=`Gathering the scriptures · ${Math.round(done/total*100)}%`;});
  $('backup-status').textContent='Your scriptures have been gathered; the backup download has begun. Keep the file somewhere safe.';log('Complete backup validated; download initiated.');
 }catch(e){$('backup-status').textContent='The scriptures could not be gathered: '+e.message;log('Backup failed: '+e.message);}
 finally{backingUp=false;render();}
});
function authorizeBundled(){
 payload=null;state.package=null;
 if(bundled&&state.authorized&&state.compatible){
  state.load(bundled);payload=bundled.image;
 }
}
async function loadBundled(){
 if(loading||state.busy||demo)return;
 const generation=++loadGeneration, expected=offering.package;
 loading=true;bundled=null;state.package=null;payload=null;
 $('retry-package').hidden=true;
 $('package-status').textContent=`Preparing ${offering.title} ${expected.version}…`;render();
 try{
  const verified=await fetchBundledPackage(expected);
  if(generation!==loadGeneration)return;
  bundled=verified;authorizeBundled();
  $('package-status').textContent=`${offering.title} ${expected.version} is ready.`;
  log(`Offering ${expected.build}: integrity verified locally. No device write.`);
 }catch(e){
  if(generation!==loadGeneration)return;
  bundled=null;state.package=null;payload=null;$('package-status').textContent=e.message;$('retry-package').hidden=false;log(e.message);
 }finally{if(generation===loadGeneration){loading=false;render();}}
}
$('retry-package').addEventListener('click',loadBundled);
$('cancel').addEventListener('click',()=>{try{state.cancel();state.wipeUserData=false;payload=null;if(access)access.onstatechange=null;log(demo?'SIMULATED cancellation.':'Cancelled before update entry. No update initiated.');}catch(e){state.fail(e);}render();});
$('install').addEventListener('click',async()=>{
 try{if(backingUp)throw Error('Let the scriptures finish downloading before beginning the rite.');const options={wipeUserData:false};state.start();render();if(demo){for(const phase of ['verify','loader','write',...(options.wipeUserData?['wipe']:[]),'reboot','done']){await new Promise(r=>setTimeout(r,600));state.step(phase,release.product);log(`SIMULATED ${phase}`);render();}return;}
 if(experimental&&(!state.compatible||release.experimentalApproval.build!==offering.package?.build))throw Error('Experimental approval does not match this device/build.');
 if(!access || matchingInputs(access).length!==1)throw Error('The connected device set changed. Reconnect exactly one FM-1 and validate again.');
 // Never mutate, repack or synthesize a package in this installer. Upstream transport validates model again.
 const step=(phase,detail,progress)=>{state.step(phase==='start'?'starting':phase,phase==='done'?detail:undefined);if(phase==='write'||phase==='verify')$('progress').removeAttribute('value');log(transferLog(phase,detail,progress));render();};
 if(state.device.model==='ota-FM-1'){
  if(!await updater.resume(payload,step,release.product,options))throw updater.lastFailure || Error('Firmware writes may be incomplete. Keep power stable and preserve the log. Check current USB state and device-specific recovery before retrying.');
 }else await updater.install(payload,release.product,step,options);
 }catch(e){state.fail(e);log(e.message);$('recovery').open=true;}render();
});
if(demo){$('demo-banner').hidden=false;$('connect').textContent='Simulate connection';$('package-status').textContent='Simulation uses a mock package; no binary is loaded.';
 const bar=document.createElement('div');bar.className='demo-banner';for(const [title,fn] of [['Mock disconnect',()=>state.disconnected()],['Mock incompatible device',()=>state.connect({model:'other',text:'OTHER_SIMULATED'})],['Mock wrong package',()=>state.load({product:'FM-1_015',sha256:'wrong',bytes:1})],['Mock write error',()=>state.fail(Error('SIMULATED cable loss during write; recovery uncertain.'))]]){const button=document.createElement('button');button.textContent=title;button.addEventListener('click',()=>{if(state.busy)return;try{fn();}catch(e){state.fail(e);}log(`SIMULATED ${title}`);render();});bar.append(button);}document.querySelector('.workflow').append(bar);
}
render();
if(!demo)loadBundled();
// Decorative liturgy deliberately isolated from all device/install state.
const blessings=[
 'May your oscillators drift and your USB connection remain boring.',
 'Forgive us our presets, as we forgive those who preset against us.',
 'You are absolved of buying another delay. Your accountant is not consulted.',
 'Let there be light. Then turn it down; the OLED is trying its best.',
 'Deliver us from menu diving, and lead us into mild modulation.',
 'Our bass, who art in headphones, hallowed be thy gain.',
 'The choir has voted. Your eight-bar loop may become a forty-minute piece.',
 'Go forth and multiply. Preferably with a ring modulator.',
 'The offering is bytes. The sacrifice is your afternoon.',
 'Blessed are the patch cables, for they shall eventually reach the correct input.'
];
let blessingIndex=0;
$('bless').addEventListener('click',()=>{$('blessing').textContent=blessings[blessingIndex++%blessings.length];});
