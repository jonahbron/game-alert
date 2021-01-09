# Game Alert Service

HTTP server written for the [Deno runtime](https://deno.land).  It has accepts two kinds of requests: `GET` and `POST.

## GET

`GET` requests return a response body of 4 single-byte characters (one for each user).  Each character can be either a `0` or a `1`.  `0` indicates that the person at that index is **not** active.  `1` indicates that the person at that index **is** active.

TODO this API should also return back the amount of time the device should sleep before polling again.

## POST

`POST` requests accept an `application/x-www-form-urlencoded` body.  The param name should be the person index being set, and the value should be their new status (either `0` or `1`).
