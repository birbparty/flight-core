/**
 * Flight-Core Platform Types - TypeScript Bindings
 * 
 * Universal platform abstraction for cross-platform development
 * from Dreamcast (16MB) to V6R cloud environments (2GB+)
 */

// Export all platform types
export * from './types';

// Re-export commonly used types for convenience
export type {
  PlatformInfo,
  PlatformCapability,
  PlatformCategory,
  PlatformFeature,
  PlatformDetection,
  PlatformMatrix,
  CompatibilityReport,
  PlatformConfig,
} from './types';

// Default export for easier importing
import * as PlatformTypes from './types';
export default PlatformTypes;
