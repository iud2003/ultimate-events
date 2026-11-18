const instructions = `You are the AI assistant for Ultimate Production, an event production company in Sri Lanka.
The company offers event lighting, laser shows, custom stage setups, and LED walls.
Answer questions about these services in the customer's language, including English, Sinhala, or Tamil.
Keep replies brief and conversational. Stay on the topic of planning an event with Ultimate Production.
For quotes and bookings, help the customer prepare the event date, venue or city, approximate guest count, and services needed. Ask only for details they haven't shared. The customer can send those details to the team using the WhatsApp link below the chat.
The team is reachable at +94 77 738 7149 or ultimatepro1995@gmail.com. You cannot contact the team or make a booking yourself.
Only the team can confirm prices, availability, and technical capabilities. Do not invent company facts or ask for payment or sensitive personal information.`;

export default {
  async fetch(request) {
    if (request.method !== 'POST') {
      return Response.json({ error: 'Use POST.' }, { status: 405, headers: { Allow: 'POST' } });
    }

    let messages;
    try {
      const body = await request.text();
      if (body.length > 16000) {
        return Response.json({ error: 'Message history is too long.' }, { status: 413 });
      }
      messages = JSON.parse(body)?.messages;
    } catch {
      return Response.json({ error: 'Invalid request.' }, { status: 400 });
    }

    if (!Array.isArray(messages) || !messages.length || messages.length > 9 ||
        messages.length % 2 === 0 || !messages.every((message, index) =>
          message?.role === (index % 2 ? 'assistant' : 'user') &&
          typeof message.content === 'string' && message.content.trim() &&
          message.content.length <= 1500)) {
      return Response.json({ error: 'Invalid chat messages.' }, { status: 400 });
    }

    if (!process.env.OPENAI_API_KEY) {
      return Response.json({ error: 'AI chat is currently unavailable.' }, { status: 503 });
    }

    try {
      const response = await fetch('https://api.openai.com/v1/responses', {
        method: 'POST',
        headers: {
          Authorization: `Bearer ${process.env.OPENAI_API_KEY}`,
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          model: 'gpt-5-mini',
          instructions,
          input: messages.map(({ role, content }) => ({ role, content })),
          reasoning: { effort: 'minimal' },
          max_output_tokens: 450,
          store: false,
        }),
        signal: AbortSignal.timeout(15000),
      });
      if (!response.ok) throw new Error(`OpenAI returned HTTP ${response.status}`);

      const data = await response.json();
      const reply = data.output
        .filter(item => item.type === 'message')
        .flatMap(item => item.content)
        .filter(part => part.type === 'output_text' || part.type === 'refusal')
        .map(part => part.text ?? part.refusal)
        .join('\n')
        .trim();
      if (!reply) throw new Error('OpenAI returned an empty reply');

      return Response.json({ reply });
    } catch (error) {
      console.error('Chat request failed:', error.message);
      return Response.json({ error: 'AI chat is currently unavailable.' }, { status: 502 });
    }
  },
};
