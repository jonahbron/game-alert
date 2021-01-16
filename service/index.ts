import {ServerRequest, serve} from 'https://deno.land/std@0.83.0/http/server.ts';
import {decode} from 'https://deno.land/std@0.83.0/encoding/utf8.ts';

/**
 * Request body application/x-www-form-urlencoded
 *
 * Valid params are 0-3, values are the new status, 0 or 1.
 */

const PARAM_PERSON = 'p';
const PARAM_STATUS = 's';

const PEOPLE_STATUS = [
  false,
  false,
  false,
  false,
];

const s = serve({port: 8000});

console.log('http://localhost:8000');

for await (const req of s) {
  switch (req.method) {
    case 'GET':
      await handleGet(req);
    break;
    case 'POST':
      await handlePost(req);
    break;
  }
}

/**
 * Returns back a body containing 4 characeters.  Each character is either a 0 or a 1.
 *
 * TODO send back the number of seconds to wait before the next poll
 */
async function handleGet(req: ServerRequest): Promise<void> {
  const statuses = PEOPLE_STATUS.map(s => s ? '1' : '0').join('')
  req.respond({
    body: `${secondsUntilNextPing()}\n${statuses}\n`,
  });
}

/**
 * Accepts parameters where keys are the person index, and the value is their new status.
 */
async function handlePost(req: ServerRequest): Promise<void> {
  const params = new URLSearchParams(decode(await Deno.readAll(req.body)));
  for (const index of PEOPLE_STATUS.keys()) {
    const param = String(index);
    if (params.has(param)) {
      const newStatus = params.get(param);
      PEOPLE_STATUS[index] = newStatus === '1';
    }
  }
  req.respond({
    body: `${secondsUntilNextPing()}\n`,
  });
}

function secondsUntilNextPing(): number {
  // One minute.
  return 1 * 60;
}
