const http = require('http');
const data = JSON.stringify({
  fullName: 'Test User',
  identityCardNumber: '123456789V',
  address: '123 Example Street',
  contactNumber: '+94 77 123 4567',
  email: 'testuser+proxy@example.com',
  password: 'password123'
});

const options = {
  hostname: 'localhost',
  port: 5175,
  path: '/api/v1/public/candidates/register',
  method: 'POST',
  headers: {
    'Content-Type': 'application/json',
    'Content-Length': Buffer.byteLength(data),
    Origin: 'http://localhost:5175'
  }
};

const req = http.request(options, (res) => {
  console.log('STATUS', res.statusCode);
  console.log('HEADERS', res.headers);
  let body = '';
  res.on('data', (chunk) => body += chunk);
  res.on('end', () => {
    console.log('BODY', body);
  });
});
req.on('error', (err) => console.error('ERROR', err.message));
req.write(data);
req.end();
