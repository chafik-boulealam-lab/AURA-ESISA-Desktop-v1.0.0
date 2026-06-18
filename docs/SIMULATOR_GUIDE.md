# AURA Simulator - Complete Feature Guide

**AURA Simulator** is an intelligent, AI-powered training platform designed to help professionals and students master technical interview skills through adaptive questioning, intelligent feedback, and comprehensive progress tracking.

---

## 📋 Feature Overview

### 🤖 AI-Powered Question Generation
- **Dynamic Questions**: Questions generated in real-time by Groq LLM
- **Three Training Domains**: 
  - 🧠 **Algorithms** - Data structures, sorting, searching, dynamic programming
  - 📐 **Mathematics** - Logic puzzles, combinatorics, probability, optimization
  - 💻 **General IT** - Networking, databases, system design, security
- **Difficulty Levels**: Easy, Medium, Hard with automatic progression
- **Concept Tracking**: Each question maps to specific learning concepts

### 📊 Intelligent Scoring & Feedback
- **AI-Driven Evaluation**: Groq LLM evaluates answers on scale of 1-10
- **Detailed Feedback**: 
  - ✅ Strengths identified
  - 📝 Constructive improvements suggested
  - 🎯 Concepts covered & missing
  - 💡 Hints for better answers
- **Real-Time Results**: Instant feedback displayed in activity log

### 📈 Progress Tracking & Analytics
- **Session Tracking**:
  - Questions answered per session
  - Success rate percentage
  - Average score across domains
  - Time tracking per question
- **Long-Term Analytics**:
  - Overall performance trends
  - Domain mastery levels
  - Concept strength mapping
  - Historical performance reports
- **Performance Report**: Comprehensive statistics accessible anytime

---

## 🎮 How AURA Simulator Works

### The Training Workflow

```
┌─────────────────────────────────────────┐
│ 1. SELECT TRAINING DOMAIN               │
│    (Algorithms / Math / IT)             │
└──────────────┬──────────────────────────┘
               ↓
┌─────────────────────────────────────────┐
│ 2. GENERATE AI QUESTION                 │
│    - Groq LLM creates custom question   │
│    - Shows difficulty level             │
│    - Provides optional hint             │
└──────────────┬──────────────────────────┘
               ↓
┌─────────────────────────────────────────┐
│ 3. ANSWER THE QUESTION                  │
│    - Type your response                 │
│    - Think through your approach        │
│    - Take your time                     │
└──────────────┬──────────────────────────┘
               ↓
┌─────────────────────────────────────────┐
│ 4. SUBMIT FOR EVALUATION                │
│    - AI evaluates your answer           │
│    - Provides score 1-10                │
│    - Lists strengths & improvements     │
└──────────────┬──────────────────────────┘
               ↓
┌─────────────────────────────────────────┐
│ 5. REVIEW FEEDBACK                      │
│    - Read AI feedback in log            │
│    - Track your progress                │
│    - See areas to improve               │
└──────────────┬──────────────────────────┘
               ↓
        [REPEAT OR VIEW REPORT]
```

---

## 🎯 Training Domains

### 1️⃣ Algorithms (Algorithmique)
**Focus**: Problem-solving, data structures, algorithmic thinking

**Sample Topics**:
- Arrays, Linked Lists, Stacks, Queues
- Trees, Graphs, Hash Tables
- Sorting & Searching algorithms
- Dynamic Programming
- Graph Traversal (BFS, DFS)
- Time/Space Complexity Analysis

**Example Question**:
> "Implement a function to detect a cycle in an undirected graph. Explain your approach and analyze the time complexity."

---

### 2️⃣ Mathematics (Énigmes Mathématiques)
**Focus**: Logic, problem-solving, mathematical reasoning

**Sample Topics**:
- Logic puzzles and riddles
- Combinatorics and probability
- Number theory basics
- Optimization problems
- Geometric reasoning
- Pattern recognition

**Example Question**:
> "You have 12 coins, one is counterfeit (either heavier or lighter). How do you find it in exactly 3 weighings using a balance scale?"

---

### 3️⃣ General IT (Culture Générale IT)
**Focus**: System design, architecture, best practices, industry knowledge

**Sample Topics**:
- Database design and optimization
- Networking protocols and architecture
- Web systems and scalability
- Security fundamentals
- Cloud computing concepts
- Distributed systems and CAP theorem
- API design and REST principles

**Example Question**:
> "Explain the CAP theorem and explain why a distributed system cannot be both consistent and highly available during a network partition."

---

## 📊 Scoring System

### Score Interpretation

| Score | Level | Meaning |
|-------|-------|---------|
| 9-10 | ⭐⭐⭐ Excellent | Comprehensive, accurate, includes all concepts |
| 7-8 | ⭐⭐ Good | Mostly accurate, minor gaps, solid understanding |
| 5-6 | ⭐ Adequate | Partially correct, missing some concepts |
| 3-4 | ❌ Weak | Significant gaps, misses important points |
| 1-2 | ❌❌ Poor | Incorrect, fundamental misunderstanding |

### Performance Metrics

**Success Rate**: % of questions scoring ≥ 7
- **80%+**: Excellent mastery
- **60-79%**: Good understanding, room for improvement
- **40-59%**: Needs practice
- **<40%**: Fundamentals need review

---

## 📈 Progress Reports

### Available Analytics

1. **Session Summary**
   - Questions answered
   - Average score
   - Success rate for this session
   - Time spent

2. **Domain Breakdown**
   - Performance per domain
   - Strong vs weak domains
   - Recommended focus areas

3. **Concept Mastery**
   - Which concepts you've mastered
   - Which concepts need work
   - Learning progression

4. **Trend Analysis**
   - Performance over time
   - Improvement trajectory
   - Consistency metrics

---

## 🚀 Getting Started

### Quick Start Guide

1. **Enter Your Profile**
   - Click "Trainee Profile" and enter your name
   - Your progress will be tracked under this profile

2. **Choose a Domain**
   - Click one of the three training tabs
   - Each domain offers unique learning paths

3. **Generate a Question**
   - Click "Generate AI Question"
   - Read the question carefully
   - Check the optional hint if needed

4. **Provide Your Answer**
   - Think through the problem
   - Type your response clearly
   - Be concise but thorough

5. **Get Feedback**
   - Click "Submit for AI Evaluation"
   - Wait for Groq to evaluate
   - Review the feedback and score

6. **Track Progress**
   - Click "Performance Report" anytime
   - See your overall statistics
   - Identify improvement areas

---

## 💡 Tips for Maximum Learning

### Best Practices

1. **Don't Rush**
   - Take time to think through each question
   - Aim for quality answers over quantity
   - Review feedback carefully

2. **Progressive Difficulty**
   - System adapts to your level
   - Start with Easy, progress through Medium, then Hard
   - Each level builds on previous knowledge

3. **Concept Mastery**
   - Pay attention to concepts_missing feedback
   - Review those concepts between sessions
   - Try similar questions again

4. **Consistent Practice**
   - Multiple short sessions better than one long session
   - Practice same domain daily for best results
   - Mix domains to maintain balance

5. **Active Learning**
   - Don't just read the feedback
   - Implement solutions to solidify learning
   - Teach concepts to others

---

## 🔒 Data & Privacy

### Storage
- All data stored locally in SQLite database
- No data sent to external servers except API calls
- Your profile and scores never leave your machine

### API Usage
- Only questions and answers sent to Groq API
- No personal identifying information sent
- Processing is stateless per request

---

## ⚙️ System Requirements

### Minimum Requirements
- Windows 7 or later
- GTK3 libraries (installed via MSYS2)
- 50MB disk space
- Internet connection (for API calls)

### Required Environment
```bash
# Install dependencies (Windows)
pacman -S --noconfirm \
  mingw-w64-x86_64-gtk3 \
  mingw-w64-x86_64-pkg-config \
  mingw-w64-x86_64-curl \
  mingw-w64-x86_64-sqlite3 \
  mingw-w64-x86_64-gcc \
  make

# Set API Key
$env:AURA_API_KEY="your_groq_api_key"
```

---

## 🆘 Troubleshooting

### Issue: "API Key not set"
**Solution**: Set environment variable before launching
```powershell
$env:AURA_API_KEY="your_key_here"
```

### Issue: "No questions generated"
**Solution**: Check internet connection, verify API key is valid

### Issue: "Very slow feedback"
**Solution**: Groq API might be experiencing high load. Try again in a few seconds.

---

## 🎓 Learning Path Recommendations

### Interview Prep
1. Start: Algorithms (Easy)
2. Progress: Algorithms (Medium → Hard)
3. Parallel: General IT (Easy → Medium)
4. Finish: Math (Medium → Hard)

### System Design Focus
1. Start: General IT (Easy)
2. Progress: General IT (Medium → Hard)
3. Mix: Math for optimization thinking
4. Master: Complex system design questions

### Competitive Programming
1. Start: Algorithms (Medium)
2. Progress: Algorithms (Hard)
3. Supplement: Math (Medium → Hard)
4. Advanced: Optimization and complexity analysis

---

## 📞 Support & Feedback

For issues or suggestions:
- Check the README.md for setup help
- Review AI_PROMPTS.md for technical details
- Verify your Groq API key is valid

---

**Version**: 1.0  
**Last Updated**: April 2026  
**Status**: Production Ready  
**License**: MIT
