// src/pages/Home.jsx

export function Home() {
  return (
    <div className="container">
      <p align="center">
        <img
          src="https://raw.githubusercontent.com/fluendo/gst.wasm/main/artwork/gst.wasm.svg"
          width="100"
          height="100"
          alt="gst.wasm logo"
        />
      </p>
      <h1 style={{ textAlign: 'center' }}>
        <pre>gst.wasm</pre>
      </h1>

      <ul className="list-group">
        <li className="list-group-item">
          <a href="gstlaunch-example/gstlaunch-example.html">gstlaunch</a>
        </li>
        <li className="list-group-item">
          <a href="openal-example/openal-example.html">openal</a>
        </li>
        <li className="list-group-item">
          <a href="videotestsrc-example/videotestsrc-example.html">videotestsrc</a>
        </li>
        <li className="list-group-item">
          <a href="webcanvassrc-example/webcanvassrc-animation-example.html">webcanvassrc (animation)</a>
        </li>
        <li className="list-group-item">
          <a href="webcanvassrc-example/webcanvassrc-webcam-example.html">webcanvassrc (webcam)</a>
        </li>
        <li className="list-group-item">
          <a href="codecs-example/codecs-example.html">webcodecs (webcanvassink)</a>
        </li>
        <li className="list-group-item">
          <a href="webdownload-example/webdownload-example.html">webcodecs (webdownload)</a>
        </li>
        <li className="list-group-item">
          <a href="gl-example/gl-example.html">OpenGL</a>
        </li>
        <li className="list-group-item">
          <a href="gstinspect-example/gstinspect-example.html">gstinspect</a>
        </li>
        <li className="list-group-item">
          <a href="lcevcdec-example/lcevcdec-example.html">lcevcdec</a>
        </li>
      </ul>
    </div>
  );
}
