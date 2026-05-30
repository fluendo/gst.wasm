import { createExample } from './createExample';

export default [
  createExample({
    id: 'webcanvassrc-animation',
    order: 9,
    title: 'Webcanvassrc Animation',
    executableName: '/webcanvassrc-example/webcanvassrc-animation-example.js',
    code: '/samples/code/webcanvassrc-animation-example.c',
  }),
  createExample({
    id: 'webcanvassrc-webcam',
    order: 10,
    title: 'Webcanvassrc Webcam',
    executableName: '/webcanvassrc-example/webcanvassrc-webcam-example.js',
    code: '/samples/code/webcanvassrc-webcam-example.c',
  }),
];
