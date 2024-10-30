import React, { useState } from 'react';
import './App.css';

function App() {
  const [query, setQuery] = useState("SELECT * FROM User WHERE id = 10"); 
  const [data, setData] = useState(null); 
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState(null);

  const fetchData = async (body) => {
    setLoading(true);
    setError(null);
    try {
      const response = await fetch('http://localhost:8080/compiler/plainText', {
        method: 'POST', 
        headers: {
          'Content-Type': 'text/plain', 
        },
        body: body, 
      });
      if (!response.ok) {
        throw new Error('Network response was not ok');
      }
      const result = await response.json(); 
      setData(result);
    } catch (error) {
      setError(error.message);
    } finally {
      setLoading(false);
    }
  };

  const handleSubmit = (e) => {
    e.preventDefault(); 
    fetchData(query); 
  };

  return (
    <div className="App">
      <header className="App-header">
        <h1>Data from Server</h1>
        <form onSubmit={handleSubmit}>
          <textarea
            value={query} 
            onChange={(e) => setQuery(e.target.value)}
            rows="4" 
            cols="50" 
            placeholder="Enter your SQL query here..."
          />
          <br />
          <button type="submit">Submit</button> 
        </form>
        {loading && <div>Loading...</div>} 
        {(error) && <div>Error: {error}</div>} 
        {(!loading && !error) && data && <pre>{JSON.stringify(data, null, 2)}</pre>}
      </header>
    </div>
  );
}

export default App;
