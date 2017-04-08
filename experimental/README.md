
http://dl.acm.org/citation.cfm?id=1029007
Slides: https://pdfs.semanticscholar.org/3681/03ce5632169531d533777966590db74008de.pdf

Concrete syntax for objects: domain-specific language embedding and assimilation without restrictions

Application programmer's interfaces give access to domain knowledge encapsulated in class libraries without providing the appropriate notation for expressing domain composition. Since object-oriented languages are designed for extensibility and reuse, the language constructs are often sufficient for expressing domain abstractions at the semantic level. However, they do not provide the right abstractions at the syntactic level. In this paper we describe MetaBorg, a method for providing <i>concrete syntax</i> for domain abstractions to application programmers. The method consists of <i>embedding</i> domain-specific languages in a general purpose host language and <i>assimilating</i> the embedded domain code into the surrounding host code. Instead of extending the implementation of the host language, the assimilation phase implements domain abstractions in terms of existing APIs leaving the host language undisturbed. Indeed, MetaBorg can be considered a method for promoting APIs to the language level. The method is supported by proven and available technology, i.e. the syntax definition formalism SDF and the program transformation language and toolset Stratego/XT. We illustrate the method with applications in three domains: code generation, XML generation, and user-interface construction.

Domain abstraction in general-purpose languages
Semantic domain abstraction
- Designed for extensibility and reuse
No syntactic domain abstraction
- Only generic syntax of method invocations
- No domain-specific notation and composition

Concrete syntax for domain abstractions
- Semantic domain abstraction
- Syntactic domain abstraction
The MetaBorg Method:
- Embedding of domain-specific language
- Assimilation of embedded domain code


