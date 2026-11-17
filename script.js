const menu=document.querySelector('.menu');const nav=document.querySelector('#navigation');menu.addEventListener('click',()=>{const open=menu.getAttribute('aria-expanded')==='true';menu.setAttribute('aria-expanded',String(!open));menu.setAttribute('aria-label',open?'Open menu':'Close menu');nav.classList.toggle('open',!open)});nav.addEventListener('click',event=>{if(event.target.closest('a')){nav.classList.remove('open');menu.setAttribute('aria-expanded','false');menu.setAttribute('aria-label','Open menu')}});

const intro=document.querySelector('.intro');
if(intro && 'IntersectionObserver' in window && !window.matchMedia('(prefers-reduced-motion: reduce)').matches){
  document.documentElement.classList.add('motion-ready');
  const observer=new IntersectionObserver((entries)=>{entries.forEach(entry=>{if(entry.isIntersecting){entry.target.classList.add('is-visible');observer.unobserve(entry.target)}})},{threshold:.12});
  observer.observe(intro);
}
