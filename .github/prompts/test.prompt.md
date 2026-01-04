---
agent: agent
---

Goal:
YOU(the copilot) should iterate through all hitbox debug markers and allows me to verify their position one by one.
This should be done in the chat interface, where I can confirm or adjust each marker's position interactively.

Desired Behavior:

1. Loop through each hitbox debug marker.
2. For each marker:
   - Ask me (the user):
     > "Is this marker in the correct location? (y/n)"
   - If I respond with **yes**, proceed to the next marker.
   - If I respond with **no**, ask:
     > "How would you like to adjust it? (e.g., delta x/y/z or new coordinates)"
3. Apply the changes I provide to the marker's position.
4. Rebuild or hot-reload the game (if needed) to reflect the changes.
5. Let me test the updated hitbox manually.
6. When I'm ready, I should be able to continue the review loop with the next marker.
