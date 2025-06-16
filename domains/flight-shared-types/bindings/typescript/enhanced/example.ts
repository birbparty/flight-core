// TypeScript Enhanced Bindings - Usage Example
import { 
  MemoryUtils, 
  useMemoryMonitor, 
  MemoryEventEmitter,
  SessionConfig 
} from '@flight/memory-enhanced';

// Basic memory utilities
const memorySize = MemoryUtils.createMemorySize(BigInt(1024 * 1024)); // 1MB
console.log('Memory size:', memorySize.humanReadable);

// Calculate memory usage
const percentage = MemoryUtils.calculateUsagePercentage({
  timestamp: Date.now(),
  sessionId: 'example',
  platform: 'v6r-medium',
  total: { bytes: BigInt(1024 * 1024 * 1024), humanReadable: '1GB' },
  used: { bytes: BigInt(512 * 1024 * 1024), humanReadable: '512MB' },
  available: { bytes: BigInt(512 * 1024 * 1024), humanReadable: '512MB' },
  fragmentationRatio: 0.1
});

console.log(`Memory usage: ${percentage.toFixed(1)}%`);

// Event system
const emitter = new MemoryEventEmitter();
const unsubscribe = emitter.subscribe('session-1', (update) => {
  console.log('Memory update received:', update.snapshot.used.humanReadable);
});

// Session configuration
const config: SessionConfig = {
  sessionId: 'user-session-1',
  userId: 'user-123',
  platform: 'v6r-medium'
};

const isValid = MemoryUtils.validateSessionConfig(config);
console.log('Config valid:', isValid);

// Clean up
unsubscribe();
