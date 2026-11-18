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

