const http = require('http');
const data = JSON.stringify({
  companyName: 'TestCo',
  admin: {
    firstName: 'Test',
    lastName: 'User',
    email: 'testuser+ci@example.com',
    password: 'password123'
  }
});

const options = {
  hostname: 'localhost',
  port: 8080,
  path: '/api/v1/auth/register',
  method: 'POST',
  headers: {
    'Content-Type': 'application/json',
    'Content-Length': Buffer.byteLength(data)
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

req.on('error', (err) => {
  console.error('ERROR', err.message);
});
req.write(data);
req.end();
