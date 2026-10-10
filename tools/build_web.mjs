import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { marked } from '/Users/Admin1/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/marked/lib/marked.esm.js';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const source = fs.readFileSync(path.join(root, '5G_PUSCH_从零开始.md'), 'utf8');
const parts = source.split(/<a id="ch(\d+)"><\/a>/);
const esc = s => s.replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const chapters = [];
for (let i = 1; i < parts.length; i += 2) {
  const n = Number(parts[i]);
  const title = parts[i+1].match(/^\s*## (.+)/)[1];
  let content = marked.parse(parts[i+1]);
  content = content.replace(/<p>(<img[^>]+alt="([^"]+)"[^>]*>)<\/p>/g,
    '<figure><button class="diagram-button" type="button" aria-label="放大图示 $2">$1</button><figcaption>点击图示可放大查看</figcaption></figure>');
  content = content.replace('<h2>', `<h2 id="heading-${n}">`);
  content = content.replace(/<table>([\s\S]*?)<\/table>/g, '<div class="table-scroll" tabindex="0" role="region" aria-label="可横向滚动的资料表"><table>$1</table></div>');
  content = content.replace(/<h3>(\d+\.\d+) 答案<\/h3>([\s\S]*?)(?=<h3>)/,
    '<details class="answers"><summary>$1 展开参考答案</summary>$2</details>');
  chapters.push({n, title, content});
}
const intro = marked.parse(parts[0].split('## 目录')[0]);
const imageCount = (source.match(/!\[/g) || []).length;
const quiz = chapters.find(c => c.title.includes('自测题'));
const questionCount = (quiz.content.match(/<details class="answers">[\s\S]*?<ol>([\s\S]*?)<\/ol>/)?.[1].match(/<li>/g) || []).length;
const summary = `${chapters.length} 章 · ${imageCount} 张图 · ${questionCount} 道自测题 · C++ 实验`;
const nav = chapters.map(c => `<a href="#ch${c.n}" data-chapter="${c.n}"><span>${String(c.n).padStart(2,'0')}</span>${esc(c.title.replace(/^\d+\s+/,''))}</a>`).join('\n');

const lab = `<div class="lab" id="resource-lab">
  <h3>动手观察 只改变 PRB 数</h3>
  <p>固定每 PRB 138 个数据 RE、单层 QPSK、目标码率 526/1024。BWP 为 50 PRB，起点为 PRB 5，因此最多分配 45 PRB；改变长度时，相应 RIV 也要重新编码。观察编码容量线性增加，而 TBS 按规则分段取值。</p>
  <div class="slider-line"><label for="prb">分配的 PRB 数</label><output id="prb-value" for="prb">10</output></div>
  <input id="prb" type="range" min="1" max="45" step="1" value="10" aria-describedby="lab-note">
  <div class="lab-values" aria-live="polite" aria-atomic="true">
    <div><span>数据 RE</span><strong id="lab-re">1380</strong></div>
    <div><span>编码比特 G</span><strong id="lab-g">2760</strong></div>
    <div><span>TBS 比特</span><strong id="lab-tbs">1416</strong></div>
    <div><span>理想 TB 吞吐</span><strong id="lab-rate">2.832 <small>Mbit/s</small></strong></div>
  </div>
  <p class="lab-formula" id="lab-formula"></p>
  <p id="lab-note">吞吐数值假设每个 0.5 ms 时隙都成功传送一个新 TB。未建模功率限制、TDD 上行比例、协议头和重传；不是手机测速预测。</p>
  <button type="button" id="lab-reset">恢复 10 PRB 算例</button>
</div>`;

let html = `<!doctype html>
<html lang="zh-CN"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="description" content="以 3GPP TS 38.211–38.215 为主，从零基础到物理层软件开发的 5G PUSCH 教程，包含 C++17 实验、收发机架构、SIMD/Linux 性能、38.901 信道与数字孪生、AI 工程。">
<title>5G PUSCH 从零开始的学习文档</title>
<style>
:root{--ink:#172c41;--muted:#617285;--blue:#215d8c;--teal:#147c75;--line:#dbe3eb;--paper:#fff;--bg:#f5f7f9;--side:292px}
*{box-sizing:border-box}html{scroll-behavior:smooth;scroll-padding-top:90px}body{margin:0;color:var(--ink);background:var(--bg);font-family:-apple-system,BlinkMacSystemFont,"PingFang SC","Microsoft YaHei","Noto Sans CJK SC",sans-serif;font-size:17px;line-height:1.9}
a{color:var(--blue);text-underline-offset:4px}a:hover{color:var(--teal)}a:focus-visible,button:focus-visible,input:focus-visible,summary:focus-visible,.table-scroll:focus-visible{outline:3px solid var(--teal);outline-offset:4px}
.top{height:64px;background:#fff;border-bottom:1px solid var(--line);position:fixed;inset:0 0 auto 0;z-index:30;display:flex;align-items:center;justify-content:space-between;padding:0 30px;gap:16px}.brand{display:flex;align-items:center;gap:13px;font-weight:650;font-size:18px;white-space:nowrap}.brand span:first-child{background:var(--ink);color:#fff;font-size:16px;line-height:32px;padding:0 10px;border-radius:4px}.top-right{display:flex;align-items:center;gap:18px;font-size:13px;color:var(--muted)}.progress{position:fixed;top:62px;left:0;height:3px;background:var(--teal);width:0;z-index:40}.menu{display:none}
.sidebar{position:fixed;left:0;top:64px;bottom:0;width:var(--side);background:#f8fafc;border-right:1px solid var(--line);padding:24px 18px 24px 22px;overflow-y:auto;z-index:20}.side-label{font-size:12px;color:var(--muted);letter-spacing:2px;margin:0 0 12px 8px}.search{width:100%;padding:11px 12px;background:#fff;border:1px solid #cbd6e0;border-radius:6px;font:inherit;font-size:14px;line-height:1.4}.search-status{font-size:12px;color:var(--muted);min-height:26px;margin:4px 8px 10px}.toc a{display:flex;gap:12px;align-items:baseline;text-decoration:none;font-size:14px;line-height:1.5;padding:10px 10px;margin:1px 0;border-radius:5px;color:#43566b}.toc a span{font-size:11px;letter-spacing:1px;color:#8090a0;min-width:20px}.toc a:hover{background:#eaf0f5}.toc a.active{background:#e7f0f6;color:var(--blue);font-weight:650}.toc a.active span{color:var(--blue)}.toc a[hidden]{display:none}.side-footer{font-size:12px;color:var(--muted);margin:22px 8px 0;border-top:1px solid var(--line);padding-top:15px}
main{margin-left:var(--side);padding:106px 48px 70px}article{max-width:960px;margin:auto;background:var(--paper);border:1px solid var(--line);padding:54px 60px 40px;border-radius:8px}.intro h1{font-size:38px;line-height:1.4;font-weight:750;letter-spacing:-.4px;color:#000;margin:0 0 18px}.intro>p:first-of-type{font-size:14px;line-height:1.8;color:var(--muted);margin-bottom:26px}.intro{padding-bottom:20px}.intro h2{font-size:21px;margin-top:32px}.chapter{border-top:1px solid var(--line);margin-top:34px;padding-top:42px}.chapter h2{font-size:27px;color:#000;line-height:1.5;letter-spacing:-.2px;margin:0 0 30px}.chapter h3{font-size:20px;color:#000;line-height:1.55;margin:34px 0 16px}p{margin:16px 0}strong{font-weight:650}ul,ol{padding-left:26px;margin:18px 0}li{margin:8px 0}li>p{margin:6px 0}pre{overflow-x:auto;padding:21px 23px;background:#f4f7fa;border:1px solid #e2e8ed;border-radius:6px;font:13px/1.9 ui-monospace,SFMono-Regular,Consolas,"PingFang SC",monospace;margin:24px 0;white-space:pre}code{font-family:ui-monospace,SFMono-Regular,Consolas,"PingFang SC",monospace;font-size:.89em;background:#f1f4f7;padding:2px 5px;border-radius:3px}pre code{font-size:inherit;background:transparent;padding:0}img{max-width:100%;height:auto;display:block;margin:29px auto 18px}.table-scroll{overflow-x:auto;margin:22px 0 28px;border:1px solid var(--line);border-radius:5px}table{border-collapse:collapse;width:100%;font-size:14px;line-height:1.75;min-width:520px}thead{background:#eaf1f6;color:#000}th{font-weight:650;text-align:left}td,th{padding:12px 14px;border-right:1px solid var(--line);border-bottom:1px solid var(--line);vertical-align:middle}td:last-child,th:last-child{border-right:0}tbody tr:last-child td{border-bottom:0}tbody tr:nth-child(even){background:#fafbfd}table code{font-size:12px;overflow-wrap:anywhere}h2,h3{break-after:avoid}img,pre,tr{break-inside:avoid}.answers{margin:28px 0;border-top:1px solid var(--line);border-bottom:1px solid var(--line);padding:18px 0}.answers summary{cursor:pointer;font-size:20px;font-weight:650;color:#000}.answers ol{margin-top:25px}.lab{margin:34px 0 12px;padding:6px 0 24px;border-top:1px solid var(--line);border-bottom:1px solid var(--line)}.lab h3{color:#000}.lab p{font-size:15px}.slider-line{display:flex;align-items:center;justify-content:space-between;margin:24px 0 8px;font-size:15px}.slider-line output{font-size:26px;font-weight:700}.lab input[type=range]{width:100%;accent-color:var(--teal);cursor:pointer;margin:10px 0 26px}.lab-values{display:grid;grid-template-columns:repeat(4,1fr);gap:18px}.lab-values>div{border-left:2px solid var(--line);padding-left:15px}.lab-values span{font-size:12px;display:block;color:var(--muted)}.lab-values strong{font-size:25px;white-space:nowrap;line-height:1.5}.lab-values small{font-size:11px;font-weight:400}.lab-formula{font-family:ui-monospace,monospace;color:var(--blue);overflow-wrap:anywhere}.lab button,.menu{background:#fff;color:var(--blue);border:1px solid #b7c9d8;border-radius:5px;padding:9px 14px;cursor:pointer;font:inherit;font-size:13px}footer{max-width:960px;margin:25px auto 0;color:var(--muted);font-size:13px;display:flex;justify-content:space-between;gap:14px}.back-top{position:fixed;right:24px;bottom:24px;border:1px solid #c6d2de;background:white;color:var(--blue);padding:9px 14px;text-decoration:none;border-radius:5px;font-size:13px;z-index:15}
figure{margin:28px 0 20px}figure img{margin:0}.diagram-button{display:block;border:0;background:#fff;padding:0;width:100%;cursor:zoom-in}figcaption{font-size:12px;color:var(--muted);text-align:center;margin-top:8px}dialog{width:calc(100% - 40px);max-width:1450px;max-height:92vh;border:1px solid var(--line);border-radius:8px;padding:20px 24px;color:var(--ink)}dialog::backdrop{background:#10263a80}.dialog-head{display:flex;align-items:center;justify-content:space-between;gap:20px}.dialog-head button{background:#fff;color:var(--blue);border:1px solid #b7c9d8;border-radius:5px;padding:7px 12px;font:inherit;font-size:14px;cursor:pointer}.dialog-help{font-size:12px;color:var(--muted);margin:8px 0}.zoom-stage{overflow:auto;max-height:75vh}.zoom-stage img{width:100%;min-width:900px;max-width:none;margin:0}
@media(min-width:1600px){main{padding-left:80px;padding-right:80px}article{max-width:1060px;padding:60px 78px}}
@media(max-width:1150px){:root{--side:250px}main{padding:94px 25px 55px}article{padding:38px 32px}.intro h1{font-size:32px}.toc a{font-size:13px}.lab-values{grid-template-columns:repeat(2,1fr)}}
@media(max-width:820px){.menu{display:block}.top{padding:0 16px}.top-right>span{display:none}.brand{font-size:15px;gap:9px}.sidebar{transform:translateX(-100%);width:min(310px,90vw);box-shadow:4px 0 18px #162c4120;transition:transform .18s}.sidebar.open{transform:translateX(0)}main{margin-left:0;padding:85px 12px 48px}article{padding:30px 21px;border-radius:6px}.intro h1{font-size:28px}.chapter{padding-top:30px}.chapter h2{font-size:23px}.chapter h3{font-size:18px}body{font-size:16px;line-height:1.85}pre{font-size:11px;padding:16px}.back-top{right:15px;bottom:15px;font-size:12px}.lab-values{gap:14px}.lab-values strong{font-size:24px}footer{padding:0 8px}.shade{display:none;position:fixed;inset:64px 0 0;background:#10263a35;z-index:19}.shade.show{display:block}}
@media(prefers-reduced-motion:reduce){html{scroll-behavior:auto}.sidebar{transition:none}}
@media print{body{background:#fff;font-size:11pt;line-height:1.65}.top,.sidebar,.progress,.back-top,.shade,.lab{display:none!important}main{margin:0;padding:0}article{border:0;padding:0;max-width:none}.chapter{padding-top:22px;margin-top:22px}.chapter h2{font-size:19pt}.chapter h3{font-size:14pt}.intro h1{font-size:24pt}.table-scroll{overflow:visible}table{font-size:9pt;min-width:0}pre{white-space:pre-wrap;font-size:8pt}a{color:inherit;text-decoration:none}details>*{display:block!important}footer{font-size:9pt}img{max-width:95%}thead{display:table-header-group}}
</style></head><body id="top">
<header class="top"><div class="brand"><span>5G</span><span>PUSCH 从入门到开发</span></div><div class="top-right"><span>38.211–38.215 · C++17 实验 · 离线可读</span><button class="menu" id="menu" type="button" aria-label="打开章节目录" aria-expanded="false" aria-controls="sidebar">目录</button></div></header>
<div class="progress" id="progress" aria-hidden="true"></div><div class="shade" id="shade"></div>
<aside class="sidebar" id="sidebar"><p class="side-label">学习目录</p><label class="visually-hidden" for="search" style="font-size:12px;color:var(--muted)">查找相关章节</label><input class="search" id="search" type="search" placeholder="输入术语 如 CRC 或 DM-RS" autocomplete="off"><p class="search-status" id="search-status">${summary}</p><nav class="toc" aria-label="章节目录">${nav}</nav><div class="side-footer">按 Ctrl / ⌘ + F 查找正文<br>图表与计算可离线查看<br>外部资料链接需要联网</div></aside>
<main><article><div class="intro">${intro}</div>${chapters.map(c => `<section class="chapter" id="ch${c.n}" aria-labelledby="heading-${c.n}">${c.content}${c.n===12?lab:''}</section>`).join('\n')}</article><footer><span>5G PUSCH 从零开始 · 开发路线更新于 2026 年 10 月 10 日</span><a href="#top">回到开头 ↑</a></footer></main><a href="#top" class="back-top" aria-label="回到文档开头">↑ 顶部</a>
<dialog id="diagram-dialog" aria-labelledby="diagram-title"><div class="dialog-head"><strong id="diagram-title">查看图示</strong><button id="diagram-close" type="button">关闭 ×</button></div><p class="dialog-help">窄屏可在图像区域内横向滚动</p><div class="zoom-stage"><img id="zoom-image" alt=""></div></dialog>
<script>
const smallTBS = [24,32,40,48,56,64,72,80,88,96,104,112,120,128,136,144,152,160,168,176,184,192,208,224,240,256,272,288,304,320,336,352,368,384,408,432,456,480,504,528,552,576,608,640,672,704,736,768,808,848,888,928,984,1032,1064,1128,1160,1192,1224,1256,1288,1320,1352,1416,1480,1544,1608,1672,1736,1800,1864,1928,2024,2088,2152,2216,2280,2408,2472,2536,2600,2664,2728,2792,2856,2976,3104,3240,3368,3496,3624,3752,3824];
function calculateTBS(ninfo, rate) {
  if (ninfo <= 3824) {
    const n = Math.max(3, Math.floor(Math.log2(ninfo))-6);
    const quantized = Math.max(24, 2**n * Math.floor(ninfo / 2**n));
    return smallTBS.find(x => x >= quantized);
  }
  const n = Math.floor(Math.log2(ninfo-24))-5;
  const quantized = Math.max(3840, 2**n * Math.round((ninfo-24)/2**n));
  if (rate <= 0.25) {
    const c = Math.ceil((quantized+24)/3816);
    return 8*c*Math.ceil((quantized+24)/(8*c))-24;
  }
  if (quantized > 8424) {
    const c = Math.ceil((quantized+24)/8424);
    return 8*c*Math.ceil((quantized+24)/(8*c))-24;
  }
  return 8*Math.ceil((quantized+24)/8)-24;
}
const slider = document.getElementById('prb');
function updateLab() {
  const p = Number(slider.value), re = 138*p, g = re*2, info = g*526/1024;
  const tbs = calculateTBS(info, 526/1024);
  document.getElementById('prb-value').textContent = p;
  document.getElementById('lab-re').textContent = re;
  document.getElementById('lab-g').textContent = g;
  document.getElementById('lab-tbs').textContent = tbs;
  document.getElementById('lab-rate').innerHTML = (tbs/500).toFixed(3)+' <small>Mbit/s</small>';
  document.getElementById('lab-formula').textContent = 'N_info = '+g+' × 526/1024 = '+info.toFixed(6)+' → TBS = '+tbs+' bit';
}
slider.addEventListener('input', updateLab);
document.getElementById('lab-reset').addEventListener('click', () => {slider.value=10;updateLab()});
updateLab();
const diagramDialog=document.getElementById('diagram-dialog');
document.querySelectorAll('.diagram-button').forEach(button=>button.addEventListener('click',()=>{
  const original=button.querySelector('img'), zoom=document.getElementById('zoom-image');
  zoom.src=original.src;zoom.alt=original.alt;
  document.getElementById('diagram-title').textContent=original.alt;diagramDialog.showModal();
}));
document.getElementById('diagram-close').addEventListener('click',()=>diagramDialog.close());
const sections = [...document.querySelectorAll('.chapter')];
const links = [...document.querySelectorAll('.toc a')];
const search = document.getElementById('search');
search.addEventListener('input', () => {
  const term=search.value.trim().toLocaleLowerCase();let count=0;
  links.forEach((a,i) => {const found=!term||sections[i].textContent.toLocaleLowerCase().includes(term);a.hidden=!found;if(found)count++});
  document.getElementById('search-status').textContent=term?'找到 '+count+' 个相关章节':'${summary}';
});
const sidebar = document.getElementById('sidebar'), menu = document.getElementById('menu'), shade = document.getElementById('shade');
function setMenu(open){sidebar.classList.toggle('open',open);shade.classList.toggle('show',open);menu.setAttribute('aria-expanded',String(open));menu.setAttribute('aria-label',open?'关闭章节目录':'打开章节目录')}
menu.addEventListener('click',()=>setMenu(!sidebar.classList.contains('open')));
shade.addEventListener('click',()=>setMenu(false));
document.addEventListener('keydown',e=>{if(e.key==='Escape')setMenu(false)});
links.forEach(a=>a.addEventListener('click',()=>setMenu(false)));
let ticking=false;
function updateScroll(){
  const max=document.documentElement.scrollHeight-innerHeight;
  document.getElementById('progress').style.width=(max>0?100*scrollY/max:0)+'%';
  let current=sections[0]?.id;sections.forEach(s=>{if(s.getBoundingClientRect().top<=135)current=s.id});
  links.forEach(a=>{const active=a.getAttribute('href')==='#'+current;a.classList.toggle('active',active);if(active)a.setAttribute('aria-current','location');else a.removeAttribute('aria-current')});
  ticking=false;
}
addEventListener('scroll',()=>{if(!ticking){requestAnimationFrame(updateScroll);ticking=true}},{passive:true});
addEventListener('resize',updateScroll);addEventListener('load',updateScroll);
</script></body></html>`;
html = html.replace(/src="((?:\/Users\/Admin1\/Desktop\/5G_PUSCH\/)?assets\/[^"\s]+\.png)"/g,
  (_, p) => {
    const png = fs.readFileSync(path.isAbsolute(p) ? p : path.join(root, p));
    return `src="data:image/png;base64,${png.toString('base64')}" width="${png.readUInt32BE(16)}" height="${png.readUInt32BE(20)}" loading="lazy" decoding="async"`;
  });
const output = path.join(root, '5G_PUSCH_从零开始.html');
// Keep source/code links usable when the HTML is served or opened with its folder.
html = html.replace(/href="\/Users\/Admin1\/Desktop\/5G_PUSCH\/([^"]+)"/g, 'href="$1"');
html = html.replace('href="PUSCH_Cpp_Labs.zip"', 'href="PUSCH_Cpp_Labs.zip" download');
fs.writeFileSync(output, html);
console.log(JSON.stringify({output, chapters:chapters.length, images:imageCount, questions:questionCount, bytes:Buffer.byteLength(html)}));
