# AURA Simulator - AI Question Generator Prompts

This document details the AI prompts used by AURA Simulator to generate questions and evaluate answers using Groq LLM.

---

## 🤖 Question Generator Prompt

This system prompt is sent to Groq LLM to generate high-quality training questions.

### Prompt Template

```
You are an expert trainer creating interview-style questions.

Generate ONE high-quality question based on:
- Domain: {Algorithmique / Mathématiques / Culture Générale IT}
- Difficulty: {Easy / Medium / Hard}

Rules:
- The question must be clear, concise, and realistic
- No multiple questions
- No explanation
- Avoid trivial questions
- Format as strict JSON with no extra text

Return format (MUST be valid JSON):
{
  "question": "Your clear and specific question here",
  "expected_concepts": ["concept1", "concept2", "concept3"],
  "difficulty": "Easy|Medium|Hard",
  "hint": "optional short hint (1 sentence max)"
}
```

### Example Outputs

#### Algorithms - Easy
```json
{
  "question": "Explain the difference between a stack and a queue. Give a real-world use case for each.",
  "expected_concepts": ["LIFO", "FIFO", "data structures", "practical applications"],
  "difficulty": "Easy",
  "hint": "Think about browser back buttons and printer queues."
}
```

#### Mathematics - Medium
```json
{
  "question": "You have 8 identical-looking coins, one of which is slightly heavier. Using a balance scale, how do you find the heavier coin in exactly 2 weighings?",
  "expected_concepts": ["problem solving", "binary search", "logic", "optimization"],
  "difficulty": "Medium",
  "hint": "Divide into groups and use information from each weighing."
}
```

#### IT Culture - Hard
```json
{
  "question": "Explain the CAP theorem (Consistency, Availability, Partition tolerance) and why a distributed system cannot satisfy all three simultaneously.",
  "expected_concepts": ["distributed systems", "CAP theorem", "trade-offs", "database design"],
  "difficulty": "Hard",
  "hint": "Consider what happens when network partitions occur."
}
```

---

## 📊 Answer Evaluation Prompt

This prompt is used to evaluate trainee answers and provide intelligent feedback.

### Prompt Template

```
You are an expert technical evaluator. Rate the following answer on a scale of 1-10.

Question: {QUESTION}
Expected Concepts: {EXPECTED_CONCEPTS}
Difficulty Level: {DIFFICULTY}
Trainee Answer: {USER_ANSWER}

Evaluation Criteria:
- 9-10: Excellent - Comprehensive, accurate, includes all key concepts with examples
- 7-8: Good - Mostly accurate with minor gaps, demonstrates solid understanding
- 5-6: Adequate - Partially correct, missing some key concepts or clarity
- 3-4: Weak - Significant gaps, misses important concepts
- 1-2: Poor - Incorrect or shows fundamental misunderstanding

Return format (MUST be valid JSON):
{
  "score": 1-10,
  "feedback": "2-3 sentence constructive feedback",
  "strengths": ["strength1", "strength2"],
  "improvements": ["area1", "area2"],
  "concepts_covered": ["covered_concept1", "covered_concept2"],
  "concepts_missing": ["missing_concept1"]
}
```

### Example Evaluation

**Input:**
```
Question: "Explain the difference between a stack and a queue."
Answer: "A stack uses LIFO (Last In First Out) and a queue uses FIFO (First In First Out). Stacks are used in browser back buttons, and queues are used in printer queues."
```

**Output:**
```json
{
  "score": 7,
  "feedback": "Good understanding of the core difference and practical examples. Could strengthen with more technical depth about use cases.",
  "strengths": ["Correctly identified LIFO vs FIFO", "Provided real-world examples"],
  "improvements": ["Discuss time complexity", "Mention implementation details (linked list vs array)"],
  "concepts_covered": ["LIFO", "FIFO", "practical applications"],
  "concepts_missing": ["performance characteristics", "array vs linked list trade-offs"]
}
```

---

## 🎯 Domain-Specific Guidance

### Algorithmique (Algorithms)
- **Difficulty Easy**: Basic data structures, simple searches, straightforward sorting
- **Difficulty Medium**: Tree traversal, graph algorithms, dynamic programming basics
- **Difficulty Hard**: Advanced graph theory, optimization problems, complex algorithms

### Mathématiques (Mathematics)
- **Difficulty Easy**: Basic arithmetic, logic puzzles, simple probability
- **Difficulty Medium**: Combinatorics, optimization, algebraic problem-solving
- **Difficulty Hard**: Proof by induction, complex probability, advanced number theory

### Culture Générale IT (General IT Knowledge)
- **Difficulty Easy**: Networking basics, database concepts, architecture fundamentals
- **Difficulty Medium**: System design, performance optimization, security basics
- **Difficulty Hard**: Distributed systems, CAP theorem, advanced architecture patterns

---

## 🔄 Difficulty Level Selection

AURA Simulator automatically selects difficulty based on trainee performance:

```
If success_rate >= 80%: Increase to next level
If success_rate < 40%: Decrease to previous level
If 40% <= success_rate < 80%: Stay at current level
```

### Progression Path
```
Easy → Medium → Hard
↑                    ↓
←── Difficulty Reset if needed ──→
```

---

## 📈 Progress Tracking Metrics

### Per-Session Metrics
- Total questions answered
- Questions correct (score ≥ 7)
- Questions incorrect (score < 7)
- Average score per domain
- Time spent per question

### Long-Term Analytics
- Overall success rate
- Domain mastery levels
- Learning velocity (improvement over time)
- Concept strength/weakness mapping
- Session history with timestamps

---

## 🔧 Implementation Details

### API Integration
AURA Simulator uses the **Groq API** for:
- Lightning-fast question generation
- Real-time answer evaluation
- Streaming responses for faster feedback

### Prompt Engineering Best Practices
1. **Strict JSON Formatting**: All responses must be valid JSON for reliable parsing
2. **Temperature Setting**: 0.7 for questions (creative but focused), 0.3 for evaluation (consistent)
3. **Max Tokens**: Question generation ≤ 500 tokens, evaluation ≤ 300 tokens
4. **Context Window**: Maintains conversation history for follow-up feedback

### Fallback Mechanisms
If API fails:
- Local pre-generated question pool loads
- Simulated scoring based on answer length heuristics
- Graceful degradation with warning message to user

---

## 🎓 Best Practices for Prompt Management

### Maintaining Quality
1. **Regular Evaluation**: Audit generated questions quarterly
2. **Feedback Loop**: Track answer distributions to detect poorly-worded questions
3. **Concept Coverage**: Ensure all important concepts are represented
4. **Difficulty Calibration**: Validate difficulty levels match learner expectations

### Customization Options (Future)
- Custom domain questions
- Industry-specific question sets
- Personalized difficulty curves
- Multi-language support

---

## 📝 JSON Schema Reference

### Question Response Schema
```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "required": ["question", "expected_concepts", "difficulty"],
  "properties": {
    "question": {
      "type": "string",
      "minLength": 20,
      "maxLength": 500
    },
    "expected_concepts": {
      "type": "array",
      "minItems": 2,
      "maxItems": 8,
      "items": {
        "type": "string"
      }
    },
    "difficulty": {
      "type": "string",
      "enum": ["Easy", "Medium", "Hard"]
    },
    "hint": {
      "type": "string",
      "maxLength": 100
    }
  }
}
```

### Evaluation Response Schema
```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "required": ["score", "feedback"],
  "properties": {
    "score": {
      "type": "integer",
      "minimum": 1,
      "maximum": 10
    },
    "feedback": {
      "type": "string",
      "minLength": 20,
      "maxLength": 300
    },
    "strengths": {
      "type": "array",
      "items": { "type": "string" }
    },
    "improvements": {
      "type": "array",
      "items": { "type": "string" }
    },
    "concepts_covered": {
      "type": "array",
      "items": { "type": "string" }
    },
    "concepts_missing": {
      "type": "array",
      "items": { "type": "string" }
    }
  }
}
```

---

## 🚀 Future Enhancements

- [ ] Adaptive difficulty based on learning patterns
- [ ] Multi-language question support
- [ ] Concept dependency tracking
- [ ] Peer comparison analytics
- [ ] Custom question generator interface
- [ ] Export/import question sets
- [ ] Real-time question quality scoring

---

**Last Updated**: April 2026  
**Version**: 1.0  
**Status**: Production Ready
