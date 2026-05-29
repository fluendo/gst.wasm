import { useState } from 'react'
import { Home } from './pages/Home';
import reactLogo from './assets/react.svg'
import viteLogo from './assets/vite.svg'
import heroImg from './assets/hero.png'
import './App.css'

function App() {
  const [count, setCount] = useState(0)

  return (
    <main>
      <Home />
    </main>
  )
}

export default App
