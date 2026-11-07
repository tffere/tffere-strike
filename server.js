import http from "node:http";
import { readFile, stat } from "node:fs/promises";
import { fileURLToPath } from "node:url";
import path from "node:path";
const root = path.resolve(fileURLToPath(new URL(".", import.meta.url)));
const port = Number(process.env.PORT || 4174);
const mime = {
  ".html": "text/html; charset=utf-8",
  ".js": "text/javascript; charset=utf-8",
  ".css": "text/css; charset=utf-8",
  ".json": "application/json",
  ".svg": "image/svg+xml",
};
http
  .createServer(async (req, res) => {
    try {
      const pathname = decodeURIComponent(
        new URL(req.url, "http://localhost").pathname,
      );
      const filename = path.resolve(
        root,
        "." + (pathname === "/" ? "/index.html" : pathname),
      );
      if (!filename.startsWith(root + path.sep) || pathname.includes("..")) {
        res.writeHead(403);
        res.end("Forbidden");
        return;
      }
      const info = await stat(filename);
      if (!info.isFile()) throw new Error("Not found");
      res.writeHead(200, {
        "Content-Type":
          mime[path.extname(filename)] || "application/octet-stream",
        "Cache-Control": "no-cache",
      });
      res.end(await readFile(filename));
    } catch {
      res.writeHead(404);
      res.end("Not found");
    }
  })
  .listen(port, "0.0.0.0", () =>
    console.log(`Tffere Strike listening on port ${port}`),
  );
