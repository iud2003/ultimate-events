const chat = document.createElement('div');
chat.className = 'event-chat';
chat.innerHTML = `
  <button class="event-chat-launch" type="button" aria-expanded="false" aria-controls="event-chat-panel">Chat with us</button>
  <section class="event-chat-panel" id="event-chat-panel" aria-labelledby="event-chat-title" hidden>
    <header class="event-chat-header">
      <h2 id="event-chat-title">Ultimate Production AI</h2>
      <button class="event-chat-close" type="button" aria-label="Close chat">×</button>
    </header>
    <div class="event-chat-log" role="log" aria-label="Conversation" tabindex="0">
      <p>Hi! Ask me about lighting, lasers, stage setups, LED walls, or what to include in a quote request.</p>
    </div>
    <p class="event-chat-status" role="status"></p>
    <form class="event-chat-form">
      <input aria-label="Your message" placeholder="Ask about your event…" maxlength="1000" required>
      <button type="submit">Send</button>
    </form>
    <a class="event-chat-handoff" href="https://wa.me/94777387149" target="_blank" rel="noopener noreferrer">Talk to the team on WhatsApp ↗</a>
  </section>
`;
document.body.append(chat);

const launcher = chat.querySelector('.event-chat-launch');
const panel = chat.querySelector('.event-chat-panel');
const log = chat.querySelector('.event-chat-log');
const status = chat.querySelector('.event-chat-status');
const form = chat.querySelector('form');
const input = form.querySelector('input');
const send = form.querySelector('button');
let history = [];

function toggleChat(open) {
  panel.hidden = !open;
  launcher.setAttribute('aria-expanded', String(open));
  (open ? input : launcher).focus();
}

launcher.addEventListener('click', () => toggleChat(panel.hidden));
chat.querySelector('.event-chat-close').addEventListener('click', () => toggleChat(false));
chat.addEventListener('keydown', event => {
  if (event.key === 'Escape' && !panel.hidden) toggleChat(false);
});

function appendMessage(content, role) {
  const message = document.createElement('p');
  message.dataset.role = role;
  message.textContent = content;
  log.append(message);
  log.scrollTop = log.scrollHeight;
  return message;
}

form.addEventListener('submit', async event => {
  event.preventDefault();
  const content = input.value.trim();
  if (!content || send.disabled) return;

  const userMessage = appendMessage(content, 'user');
  const messages = [...history, { role: 'user', content }];
  input.value = '';
  send.disabled = true;
  status.textContent = 'Preparing a reply…';

  try {
    const response = await fetch('api/chat', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ messages }),
      signal: AbortSignal.timeout(20000),
    });
    if (!response.ok) throw new Error('Chat unavailable');
    const { reply } = await response.json();
    appendMessage(reply, 'assistant');
    history = [...messages, { role: 'assistant', content: reply.slice(0, 1500) }].slice(-8);
    status.textContent = '';
  } catch {
    userMessage.remove();
    if (!input.value) input.value = content;
    status.textContent = 'AI chat is unavailable. Try again or message the team on WhatsApp.';
  } finally {
    send.disabled = false;
  }
});
