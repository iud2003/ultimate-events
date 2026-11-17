const menu=document.querySelector('.menu');const nav=document.querySelector('#navigation');menu.addEventListener('click',()=>{const open=menu.getAttribute('aria-expanded')==='true';menu.setAttribute('aria-expanded',String(!open));menu.setAttribute('aria-label',open?'Open menu':'Close menu');nav.classList.toggle('open',!open)});nav.addEventListener('click',event=>{if(event.target.closest('a')){nav.classList.remove('open');menu.setAttribute('aria-expanded','false');menu.setAttribute('aria-label','Open menu')}});

const intro=document.querySelector('.intro');
if(intro && 'IntersectionObserver' in window && !window.matchMedia('(prefers-reduced-motion: reduce)').matches){
  document.documentElement.classList.add('motion-ready');
  const observer=new IntersectionObserver((entries)=>{entries.forEach(entry=>{if(entry.isIntersecting){entry.target.classList.add('is-visible');observer.unobserve(entry.target)}})},{threshold:.12});
  observer.observe(intro);
}

const stats=document.querySelector('.stats');
if(stats && 'IntersectionObserver' in window && !window.matchMedia('(prefers-reduced-motion: reduce)').matches){
  const counterObserver=new IntersectionObserver((entries)=>{
    if(!entries[0].isIntersecting)return;
    counterObserver.disconnect();
    stats.querySelectorAll('[data-count]').forEach(counter=>{
      const target=Number(counter.dataset.count);
      const number=counter.firstChild;
      const duration=1800;
      let start;
      function update(time){
        if(start===undefined)start=time;
        const progress=Math.min((time-start)/duration,1);
        const eased=1-Math.pow(1-progress,3);
        number.nodeValue=Math.round(target*eased).toLocaleString('en-US');
        if(progress<1)requestAnimationFrame(update);
        else number.nodeValue=target.toLocaleString('en-US');
      }
      requestAnimationFrame(update);
    });
  },{threshold:.35});
  counterObserver.observe(stats);
}

if(intro && !window.matchMedia('(prefers-reduced-motion: reduce)').matches){
  let truckFramePending=false;
  const updateTruck=()=>{
    const bounds=intro.getBoundingClientRect();
    const viewport=window.innerHeight;
    const progress=Math.max(0,Math.min(1,(viewport-bounds.top)/(viewport+bounds.height)));
    const distance=Math.min(window.innerWidth*.28,360);
    intro.style.setProperty('--truck-shift',`${-progress*distance}px`);
    truckFramePending=false;
  };
  const requestTruckUpdate=()=>{
    if(!truckFramePending){truckFramePending=true;requestAnimationFrame(updateTruck)}
  };
  window.addEventListener('scroll',requestTruckUpdate,{passive:true});
  window.addEventListener('resize',requestTruckUpdate);
  requestTruckUpdate();
}
