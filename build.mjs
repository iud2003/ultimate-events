import { cp, mkdir } from 'node:fs/promises';

await mkdir('dist', { recursive: true });
for (const file of ['index.html', 'clients.html', 'style.css', 'script.js']) {
  await cp(file, `dist/${file}`);
}
await cp('assets', 'dist/assets', { recursive: true });
console.log('Static site ready in dist/');
