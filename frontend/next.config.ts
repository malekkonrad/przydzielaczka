import type { NextConfig } from 'next';

const isStatic = process.env.STATIC_BUILD === 'true';
const basePath = process.env.NEXT_PUBLIC_BASE_PATH ?? '';

const nextConfig: NextConfig = {
  ...(isStatic && { output: 'export' }),
  ...(basePath && { basePath }),
  // headers() is ignored in static export mode — coi-serviceworker handles COOP/COEP on GitHub Pages
  ...(!isStatic && {
    async headers() {
      return [
        {
          source: '/wasm/:path*',
          headers: [
            { key: 'Cross-Origin-Opener-Policy', value: 'same-origin' },
            { key: 'Cross-Origin-Embedder-Policy', value: 'require-corp' },
          ],
        },
      ];
    },
  }),
};

export default nextConfig;
